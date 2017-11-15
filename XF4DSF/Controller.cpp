#include <memory>
#include <sstream>
#include <string>

#include "Includes.h"
#include "Controller.h"
#include "Console.h"
#include "Fallout.h"

const double micros = 1.0E6;

namespace {

	struct ControllerConfig {
		int		bSimpleMode;
		int		bShowDiagnostics;
		int		bCheckPipboy;
		int		bCheckPaused;
		int		bLoadCapping;
		float	fTargetFPS;
		float	fEmergencyDropFPS;
		float	fShadowDirDistanceMin;
		float	fShadowDirDistanceMax;
		float	fTargetLoad;
		int		bAdjustGRQuality;
		int		iGRQualityMin;
		int		iGRQualityMax;
		float	fGRQualityShadowDist[3];
		bool	bUsePreciseCapping;
	};
}

class Clock
{
private:
	double timeToMicros;
public:
	Clock() {
		LARGE_INTEGER _frequency;
		timeBeginPeriod(1);
		QueryPerformanceFrequency(&_frequency);
		timeToMicros = micros / _frequency.QuadPart;
	}

	double getMicrosPerTick() const {
		return timeToMicros;
	}

	microsecond_t currentMicrosecond()
	{
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);

		return (microsecond_t)(time.QuadPart * timeToMicros);
	}

	void sleepUntil(microsecond_t time) {
		const microsecond_t timeRemaining = time - currentMicrosecond();
		const millisecond_t sleepTimeInt = millisecond_t(timeRemaining / 1000);
		if (sleepTimeInt <= 0) {
			return;
		}
		Sleep(DWORD(sleepTimeInt));
	}
	
	void preciseSleepUntil(microsecond_t time) {
		sleepUntil(time);
		while (currentMicrosecond() < time) {
		}
	}
} Clock;


class AdvancedController
{
private:
	bool loadCapping;
	const microsecond_t targetFrameTime;
	const float minDistance;
	const float maxDistance;
	const float targetLoad;
	float momentum;
	const microsecond_t emergencyDrop;
	float load;
	const bool debugEnabled;
	const microsecond_t debugMessageDelay;
	float volumetricDistances[3];
	std::unique_ptr<Fallout4> fallout4;

	microsecond_t lastFrameStart;
	microsecond_t nextDebugTime;

	bool shouldCap() {
		return loadCapping || !fallout4->isGameLoading();
	}

	bool shouldAdjust() {
		return !(fallout4->isMainMenu() || fallout4->isGamePaused() || fallout4->isGameLoading());
	}

	void adjust()
	{
		if (targetLoad > load) {
			if (momentum < 0) {
				momentum = 1;
			}
			else {
				momentum *= 1.025;
				if (momentum > 500) {
					momentum = 500;
				}
			}
		}
		else {
			if (momentum > 0) {
				momentum = -2;
			}
			else {
				momentum *= 1.025;
				if (momentum < -500) {
					momentum = -500;
				}
			}
		}
		const float newDistance = max(minDistance, min(maxDistance, fallout4->getShadowDirDistance() + momentum));
		fallout4->setShadowDirDistance(newDistance);
		fallout4->setVolumetricQuality(getVolumetricQuality(newDistance)); 
	}

	int getVolumetricQuality(float distance)
	{
		for (int i = 0; i < 3; ++i) {
			if (distance < volumetricDistances[i]) {
				return i;
			}
		}
		return 3;
	}

	bool shouldDisplayDebug() {
		return debugEnabled && nextDebugTime < lastFrameStart;
	}
	
public:
	AdvancedController(bool loadCapping, microsecond_t targetFrameTime, float minDistance, float maxDistance, float targetLoad, microsecond_t emergencyDrop, float _volumetricDistances[3], bool checkPipboy, bool debugEnabled) : 
		loadCapping(loadCapping), 
		targetFrameTime(targetFrameTime), 
		minDistance(minDistance), 
		maxDistance(maxDistance), 
		targetLoad(targetLoad), 
		momentum(1), 
		emergencyDrop(emergencyDrop), 
		load(0), 
		debugEnabled(debugEnabled), 
		debugMessageDelay(microsecond_t(micros)),
		fallout4(std::make_unique<Fallout4>()),
		lastFrameStart(0),
		nextDebugTime(0)
	{
		memcpy(volumetricDistances, _volumetricDistances, sizeof(float) * 3);
	}

	void tick()
	{
		if (lastFrameStart == 0) {
			lastFrameStart = Clock.currentMicrosecond();
			return;
		}

		const microsecond_t workCompleted = Clock.currentMicrosecond();
		const microsecond_t currentWorkTime = workCompleted - lastFrameStart;
		
		load = float(currentWorkTime) / targetFrameTime;

		if (shouldAdjust()) {
			adjust();
		}

		microsecond_t sleepTime = 0;

		if (shouldCap()) {
			sleepTime = lastFrameStart + targetFrameTime - workCompleted;
			Clock.preciseSleepUntil(lastFrameStart + targetFrameTime);
		}


		if (shouldDisplayDebug()) {
			const microsecond_t totalFrameTime = Clock.currentMicrosecond() - lastFrameStart;
			const int BUF_LEN = 256;
			char buf[BUF_LEN];
			sprintf_s(buf, BUF_LEN, "D%6d(%+6.1f) GR%1d %+5llduS work %+5llduS sleep =%+5llduS frame. %3.0f%% of target %5lldUS, %4s %5s %4s.\n",
				(int)fallout4->getShadowDirDistance(), momentum, fallout4->getVolumetricQuality(), currentWorkTime, sleepTime, totalFrameTime, load * 100, targetFrameTime, fallout4->isMainMenu() ? "Menu" : "" ,fallout4->isGamePaused() != 0 ? "Paused" : "Running", fallout4->isGameLoading() != 0 ? "Load" : "");
			console.print(buf);
			nextDebugTime = Clock.currentMicrosecond() + debugMessageDelay;
		}

		lastFrameStart = Clock.currentMicrosecond();
	}
};


std::unique_ptr<AdvancedController> createController(const char *configPath)
{
	ControllerConfig c;
	c.fTargetFPS = 60.0f;
	c.fEmergencyDropFPS = 58.0f;
	c.fShadowDirDistanceMin = 2500.0f;
	c.fShadowDirDistanceMax = 12000.0f;
	c.bAdjustGRQuality = TRUE;
	c.iGRQualityMin = 0;
	c.iGRQualityMax = 3;
	c.bSimpleMode = FALSE;
	c.bShowDiagnostics = FALSE;
	c.fTargetLoad = 98.0f;
	c.fGRQualityShadowDist[0] = 3000.0f;
	c.fGRQualityShadowDist[1]= 6000.0f;
	c.fGRQualityShadowDist[2] = 10000.0f;
	c.bCheckPipboy = TRUE;
	c.bCheckPaused = TRUE;
	c.bLoadCapping = FALSE;
	c.bUsePreciseCapping = TRUE;

	FILE* p = NULL;
	fopen_s(&p, configPath, "rt");
	if (p != NULL) {
		const int MAX_LINES = 96;
		const int MAX_LINE_CHARS = 256;
		fseek(p, 0, SEEK_SET);
		for (int i = 0; i < MAX_LINES; ++i) {
			char lineBuf[MAX_LINE_CHARS];
			fgets(lineBuf, MAX_LINE_CHARS, p);

			if (sscanf_s(lineBuf, "fTargetFPS=%f", &c.fTargetFPS) > 0) {
				c.fTargetFPS = max(0.0f, c.fTargetFPS);
			}

			if (sscanf_s(lineBuf, "fEmergencyDropFPS=%f", &c.fEmergencyDropFPS) > 0) {
				c.fEmergencyDropFPS = max(0.0f, c.fEmergencyDropFPS);
			}

			if (sscanf_s(lineBuf, "fTargetLoad=%f", &c.fTargetLoad) > 0) {
			}

			if (sscanf_s(lineBuf, "fShadowDirDistanceMin=%f", &c.fShadowDirDistanceMin) > 0) {
				c.fShadowDirDistanceMin = max(0.0f, c.fShadowDirDistanceMin);
			}

			if (sscanf_s(lineBuf, "fShadowDirDistanceMax=%f", &c.fShadowDirDistanceMax) > 0) {
				c.fShadowDirDistanceMax = max(0.0f, c.fShadowDirDistanceMax);
			}

			if (sscanf_s(lineBuf, "fGRQualityShadowDist1=%f", &c.fGRQualityShadowDist[0]) > 0) {
				c.fGRQualityShadowDist[0] = max(0.0f, c.fGRQualityShadowDist[0]);
			}

			if (sscanf_s(lineBuf, "fGRQualityShadowDist2=%f", &c.fGRQualityShadowDist[1]) > 0) {
				c.fGRQualityShadowDist[1] = max(0.0f, c.fGRQualityShadowDist[1]);
			}

			if (sscanf_s(lineBuf, "fGRQualityShadowDist3=%f", &c.fGRQualityShadowDist[2]) > 0) {
				c.fGRQualityShadowDist[2] = max(0.0f, c.fGRQualityShadowDist[2]);
			}

			if (sscanf_s(lineBuf, "bAdjustGRQuality=%d", &c.bAdjustGRQuality) > 0) {
				c.bAdjustGRQuality = (c.bAdjustGRQuality != 0) ? 1 : 0;
			}

			//if (sscanf_s(lineBuf, "bSimpleMode=%d", &c.bSimpleMode) > 0) {
				//c.bSimpleMode = (c.bSimpleMode != 0) ? 1 : 0;
			//}

			if (sscanf_s(lineBuf, "bShowDiagnostics=%d", &c.bShowDiagnostics) > 0) {
				c.bShowDiagnostics = (c.bShowDiagnostics != 0) ? 1 : 0;
			}
			
			if (sscanf_s(lineBuf, "bLoadCapping=%d", &c.bLoadCapping) > 0) {
				c.bLoadCapping = (c.bLoadCapping != 0) ? 1 : 0;
			}
		}
		fclose(p);
	}
	if (c.bShowDiagnostics) {
		console.print("xwize's Dynamic Performance Tuner is running, with Stochastic Tinker's modifications.");
	}
	return std::make_unique<AdvancedController>(c.bLoadCapping != FALSE, microsecond_t(micros/c.fTargetFPS), c.fShadowDirDistanceMin, c.fShadowDirDistanceMax, c.fTargetLoad/100.0f, microsecond_t(micros/c.fEmergencyDropFPS), c.fGRQualityShadowDist, c.bCheckPipboy != FALSE, c.bShowDiagnostics != FALSE);
}

Controller::Controller(const char *configPath) : controllerImpl(createController(configPath))
{}

Controller::~Controller() {}

void Controller::tick() {
	controllerImpl->tick();
}
