#include <memory>
#include <strstream>
#include <string>

#include "Includes.h"
#include "Controller.h"
#include "Console.h"
#include "Fallout.h"

const float micros = 1e6f;

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
	__int64 frequency;
public:
	Clock() {
		LARGE_INTEGER _frequency;
		QueryPerformanceFrequency(&_frequency);
		frequency = _frequency.QuadPart;
	}

	microsecond_t currentMicrosecond()
	{
		LARGE_INTEGER time;
		QueryPerformanceCounter(&time);

		return (microsecond_t)(micros * time.QuadPart / float(frequency));
	}

	void sleepUntil(microsecond_t time) {
		const microsecond_t timeRemaining = time - currentMicrosecond();
		const millisecond_t sleepTimeInt = (millisecond_t)(timeRemaining / 1000);
		if (sleepTimeInt <= 0) {
			return;
		}
		Sleep(DWORD(sleepTimeInt));

	}
	
	void preciseSleepUntil(microsecond_t time) {
		sleepUntil(time - 500);
		while (currentMicrosecond() < time) {
		}
	}
} Clock;


class TimeRange
{
private:
	microsecond_t startTime;
	microsecond_t endTime;
public:
	TimeRange() : startTime(-1), endTime(-1) {}

	void start() {
		startTime = Clock.currentMicrosecond();
	}

	void end() {
		endTime = Clock.currentMicrosecond();
	}

	microsecond_t getStartTime() const {
		return startTime;
	}

	microsecond_t getEndTime() const {
		return startTime;
	}

	microsecond_t value() const {
		return endTime - startTime;
	}

	bool hasValue() const {
		return endTime >= startTime && startTime > 0 && endTime > 0;
	}
};

class TimeRangeAverager
{
private:
	int maxSamples;
	microsecond_t rollingTotal;
	int currentSampleOffset;
	int totalSamples;
	std::unique_ptr<millisecond_t[]> frameTimes;
public:
	TimeRangeAverager(int maxSamples) : maxSamples(maxSamples),
		rollingTotal(0),
		currentSampleOffset(0),
		totalSamples(0),
		frameTimes(new millisecond_t[maxSamples])
	{
		std::fill_n(frameTimes.get(), maxSamples, 0);
	}

	void sample(const TimeRange &sampler) {
		if (sampler.hasValue()) {
			const microsecond_t currentTimeFrame = sampler.value();
			if (totalSamples < maxSamples) {
				totalSamples++;
			}
			rollingTotal = rollingTotal - frameTimes[currentSampleOffset] + currentTimeFrame;
			frameTimes[currentSampleOffset] = currentTimeFrame;
			if (++currentSampleOffset >= maxSamples) {
				currentSampleOffset = 0;
			}
		}
	}

	float getAverage() const {
		return float(rollingTotal) / totalSamples;
	}

	bool hasSamples() const {
		return totalSamples != 0;
	}
};

class AdvancedController
{
private:
	bool loadCapping;

	TimeRangeAverager averageWorkTime;
	TimeRangeAverager averageFrameTime;
	TimeRange currentFrameTime;
	TimeRange lastDebugMessage;
	microsecond_t targetFrameTime;
	float minDistance;
	float maxDistance;
	float targetLoad;
	float momentum;
	microsecond_t emergencyDrop;
	float load;
	bool checkPipboy;
	bool debugEnabled;
	microsecond_t debugMessageDelay;

	std::unique_ptr<Fallout4> fallout4;

	bool shouldCap() {
		return loadCapping || !fallout4->isGameLoading();
	}

	bool shouldAdjust() {
		return !(fallout4->isGamePaused() || fallout4->isGameLoading() || (checkPipboy && fallout4->isPipboyActive()));
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
		fallout4->setShadowDirDistance(max(minDistance, min(maxDistance, fallout4->getShadowDirDistance() + momentum)));
	}

	bool shouldDisplayDebug() {
		return debugEnabled && !lastDebugMessage.hasValue() || lastDebugMessage.value() > debugMessageDelay;
	}
	
public:
	AdvancedController(bool loadCapping, microsecond_t targetFrameTime, float minDistance, float maxDistance, float targetLoad, microsecond_t emergencyDrop, bool checkPipboy, bool debugEnabled) : 
		loadCapping(loadCapping), 
		averageWorkTime(20), 
		averageFrameTime(20), 
		currentFrameTime(), 
		lastDebugMessage(), 
		targetFrameTime(targetFrameTime), 
		minDistance(minDistance), 
		maxDistance(maxDistance), 
		targetLoad(targetLoad), 
		momentum(1), 
		emergencyDrop(emergencyDrop), 
		load(0), 
		checkPipboy(checkPipboy),
		debugEnabled(debugEnabled), 
		debugMessageDelay(micros/6),
		fallout4(std::make_unique<Fallout4>())
	{}

	void tick()
	{
		currentFrameTime.end();
		if (currentFrameTime.hasValue()) {
			averageWorkTime.sample(currentFrameTime);
			load = averageWorkTime.getAverage() / targetFrameTime;

			if (shouldAdjust()) {
				adjust();
			}

			if (shouldCap()) {
				Clock.preciseSleepUntil(currentFrameTime.getStartTime() + targetFrameTime);
			}

			lastDebugMessage.end();
			if (shouldDisplayDebug()) {
				const int BUF_LEN = 256;
				char buf[BUF_LEN];
				//                         1    2        3     4    5        6     7    9  10   
				sprintf_s(buf, BUF_LEN, "D%6d(%+6.1f) GR%1d %5.1f/%5.1f = %3.0f%%, %8s %5s %4s.  \n",
					// 1									   2			      	3								 4										5									6		7								9										 10											11
					(int)fallout4->getShadowDirDistance(), momentum, fallout4->getVolumetricQuality(), micros / averageWorkTime.getAverage(), micros / averageFrameTime.getAverage(), load*100, fallout4->isPipboyActive() != 0 ? "Pip-Boy" : "", fallout4->isGamePaused() != 0 ? "Pause" : "", fallout4->isGameLoading() != 0 ? "Load" : "");
				console.print(buf);
				lastDebugMessage.start();
			}
			
			currentFrameTime.end();
			averageFrameTime.sample(currentFrameTime);
		}
		currentFrameTime.start();
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

	return std::make_unique<AdvancedController>(c.bLoadCapping != FALSE, microsecond_t(micros/c.fTargetFPS), c.fShadowDirDistanceMin, c.fShadowDirDistanceMax, c.fTargetLoad/100.0f, microsecond_t(micros/c.fEmergencyDropFPS), c.bCheckPipboy != FALSE, c.bShowDiagnostics != FALSE);
}

Controller::Controller(const char *configPath) : controllerImpl(createController(configPath))
{}

Controller::~Controller() {}

void Controller::tick() {
	controllerImpl->tick();
}
