#include <memory>
#include <sstream>
#include <string>

#include "Includes.h"
#include "Controller.h"
#include "Console.h"
#include "Fallout.h"

namespace
{
	const double micros = 1.0E6;

	float GetPrivateProfileFloatA(LPCSTR lpAppName, LPCSTR lpKeyName, float defaultValue, LPCSTR lpFileName)
	{
		char result[256];
		GetPrivateProfileStringA(lpAppName, lpKeyName, "", result, 256, lpFileName);
		_set_errno(0);
		float parsed = float(atof(result));
		if (errno)
		{
			return defaultValue;
		}

		return parsed;
	}
}

class Clock
{
private:
	double timeToMicros;
public:
	Clock()
	{
		LARGE_INTEGER _frequency;
		timeBeginPeriod(1);
		QueryPerformanceFrequency(&_frequency);
		timeToMicros = micros / _frequency.QuadPart;
	}

	double getMicrosPerTick() const
	{
		return timeToMicros;
	}

	microsecond_t currentMicrosecond()
	{
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);

		return (microsecond_t)(time.QuadPart * timeToMicros);
	}

	void sleepUntil(microsecond_t time)
	{
		const microsecond_t timeRemaining = time - currentMicrosecond();
		const millisecond_t sleepTimeInt = millisecond_t(timeRemaining / 1000);
		if (sleepTimeInt <= 0)
		{
			return;
		}
		Sleep(DWORD(sleepTimeInt));
	}

	void preciseSleepUntil(microsecond_t time)
	{
		sleepUntil(time);
		while (currentMicrosecond() < time)
		{
		}
	}
} Clock;


class AdvancedController
{
private:
	std::shared_ptr<Fallout4> fallout4;
	const bool usePreciseSleep;
	const bool loadCapping;
	const bool capFramerate;
	const microsecond_t targetFrameTime;
	const float minDistance;
	const float maxDistance;
	const float targetLoad;
	float momentum;
	float load;
	const bool debugEnabled;
	const microsecond_t debugMessageDelay;
	const bool updateVolumetric;
	float volumetricDistances[3];

	microsecond_t lastFrameStart;
	microsecond_t nextDebugTime;

	bool shouldCap()
	{
		return capFramerate && (loadCapping || !fallout4->isGameLoading());
	}

	bool shouldAdjust()
	{
		return !(fallout4->isMainMenu() || fallout4->isGamePaused() || fallout4->isGameLoading());
	}

	void adjust()
	{
		if (targetLoad > load)
		{
			if (momentum < 0)
			{
				momentum = 1;
			}
			else
			{
				momentum *= 1.025;
				if (momentum > 500)
				{
					momentum = 500;
				}
			}
		}
		else
		{
			if (momentum > 0)
			{
				momentum = -2;
			}
			else
			{
				momentum *= 1.025;
				if (momentum < -500)
				{
					momentum = -500;
				}
			}
		}
		const float newDistance = max(minDistance, min(maxDistance, fallout4->getShadowDirDistance() + momentum));
		fallout4->setShadowDirDistance(newDistance);
		if (updateVolumetric)
		{
			fallout4->setVolumetricQuality(getVolumetricQuality(newDistance));
		}
	}

	int getVolumetricQuality(float distance)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (distance < volumetricDistances[i])
			{
				return i;
			}
		}
		return 3;
	}

	bool shouldDisplayDebug()
	{
		return debugEnabled && nextDebugTime < lastFrameStart;
	}

public:
	AdvancedController(std::shared_ptr<Fallout4> fallout4, bool loadCapping, bool usePreciseSleep, bool capFramerate, microsecond_t targetFrameTime, float minDistance, float maxDistance,
	                   float targetLoad, bool updateVolumetric, float _volumetricDistances[3], bool debugEnabled) :
		fallout4(fallout4),
		usePreciseSleep(usePreciseSleep),
		loadCapping(loadCapping),
		capFramerate(capFramerate),
		targetFrameTime(targetFrameTime),
		minDistance(minDistance),
		maxDistance(maxDistance),
		targetLoad(targetLoad),
		momentum(1),
		load(0),
		debugEnabled(debugEnabled),
		debugMessageDelay(microsecond_t(micros)),
		updateVolumetric(updateVolumetric),
		lastFrameStart(0),
		nextDebugTime(0)
	{
		memcpy(volumetricDistances, _volumetricDistances, sizeof(float) * 3);
	}


	void tick()
	{
		if (lastFrameStart == 0)
		{
			lastFrameStart = Clock.currentMicrosecond();
			return;
		}

		const microsecond_t workCompleted = Clock.currentMicrosecond();
		const microsecond_t currentWorkTime = workCompleted - lastFrameStart;

		load = float(currentWorkTime) / targetFrameTime;

		if (shouldAdjust())
		{
			adjust();
		}

		microsecond_t sleepTime = 0;

		if (shouldCap())
		{
			sleepTime = lastFrameStart + targetFrameTime - workCompleted;
			if (usePreciseSleep)
			{
				Clock.preciseSleepUntil(lastFrameStart + targetFrameTime);
			} 
			else
			{
				Clock.sleepUntil(lastFrameStart + targetFrameTime);
			}
		}

		if (shouldDisplayDebug())
		{
			const microsecond_t totalFrameTime = Clock.currentMicrosecond() - lastFrameStart;
			const int BUF_LEN = 256;
			char buf[BUF_LEN];
			sprintf_s(buf, BUF_LEN,
			          "D%6d(%+6.1f) GR%1d (work+sleep=frame: %5.1fms + %5.1fms = %5.1fms). %3.0f%% of target %5lldms, %7s %7s %4s.\n",
			          (int)fallout4->getShadowDirDistance(), momentum, fallout4->getVolumetricQuality(), currentWorkTime/1000.0f,
			          sleepTime/1000.0f, totalFrameTime/1000.0f, load * 100, targetFrameTime/1000, fallout4->isMainMenu() ? "In Menu" : "",
			          fallout4->isGamePaused() != 0 ? "Paused" : "Running", fallout4->isGameLoading() != 0 ? "Load" : "");
			console.print(buf);
			nextDebugTime = Clock.currentMicrosecond() + debugMessageDelay;
		}

		lastFrameStart = Clock.currentMicrosecond();
	}
};


std::unique_ptr<AdvancedController> make_controller(std::string configFile, std::shared_ptr<Fallout4> fallout4)
{
	auto configPath = configFile.c_str();
	auto fTargetFPS = GetPrivateProfileFloatA("Simple", "fTargetFPS", 60.0, configPath);
	auto bCapFramerate = GetPrivateProfileIntA("Simple", "bCapFramerate", TRUE, configPath);
	auto fTargetLoad = GetPrivateProfileFloatA("Simple", "fTargetLoad", 98, configPath);
	auto fShadowDirDistanceMin = max(0.0f, GetPrivateProfileFloatA("Simple", "fShadowDirDistanceMin", 2500.0f, configPath));
	auto fShadowDirDistanceMax = max(0.0f, GetPrivateProfileFloatA("Simple", "fShadowDirDistanceMax", 12000.0f, configPath));

	auto bAdjustGRQuality = GetPrivateProfileIntA("Simple", "bAdjustGRQuality", TRUE, configPath);
	auto bShowDiagnostics = GetPrivateProfileIntA("Advanced", "bShowDiagnostics", FALSE, configPath);
	float fGRQualityShadowDist[3];
	fGRQualityShadowDist[0] = GetPrivateProfileFloatA("Simple", "fGRQualityShadowDist1", 3000, configPath);
	fGRQualityShadowDist[1] = GetPrivateProfileFloatA("Simple", "fGRQualityShadowDist2", 6000, configPath);
	fGRQualityShadowDist[2] = GetPrivateProfileFloatA("Simple", "fGRQualityShadowDist3", 10000, configPath);

	auto bLoadCapping = GetPrivateProfileIntA("Advanced", "bLoadCapping", FALSE, configPath);
	auto bUsePreciseCapping = GetPrivateProfileIntA("Advanced", "bUsePreciseCapping", FALSE, configPath);


	if (bShowDiagnostics)
	{
		console.print("Dynamic Performance Tuner is running.\n");
		std::stringstream details;
		details
			<< "  Target FPS:            " << fTargetFPS << std::endl
			<< "  Max Framerate:         " << (bCapFramerate ? "Locked" : "Unlocked") << std::endl
			<< "  Sleep precision:       " << (bUsePreciseCapping ? "Microsecond" : "Millisecond") << std::endl
			<< "  Target Load:           " << fTargetLoad << std::endl
			<< "  Load Screen:           " << (bLoadCapping ? "Normal" : "Accelerated") << std::endl
			<< "  Shadow Distance Range: (" << fShadowDirDistanceMax << " to " << fShadowDirDistanceMax << ")" << std::endl
			<< "  Godray Quality:        " << (bAdjustGRQuality ? "Adjusting" : "Constant") << std::endl;

		if (bAdjustGRQuality)
		{
			for (int i = 0; i < 3; ++i)
			{
				details << "   Quality Level " << (i + 1) << " Distance: " << fGRQualityShadowDist[i] << std::endl;
			}
		}
		console.print(details.str());
	}

	return std::make_unique<AdvancedController>(fallout4, bLoadCapping != FALSE, bUsePreciseCapping != FALSE, bCapFramerate != FALSE, microsecond_t(micros / fTargetFPS),
	                                            fShadowDirDistanceMin, fShadowDirDistanceMax, fTargetLoad / 100.0f,
	                                            bAdjustGRQuality != FALSE, fGRQualityShadowDist, bShowDiagnostics != FALSE);
}

Controller::Controller(std::string configPath, std::shared_ptr<Fallout4> fallout4) : controllerImpl(make_controller(configPath, fallout4))
{
}

Controller::~Controller()
{
}

void Controller::tick()
{
	controllerImpl->tick();
}
