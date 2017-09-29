#include "Includes.h"
#include "Controller.h"
#include "Fallout.h"

#define PRECISE_CAP

config_t config;

// Constants
const float usecInSec = 10e5f;
const float pi = 3.14159f;
const float sqrt_pi = 1.772f;

void DebugPrint(const char* str) {
	static HANDLE handle = 0;
	if (handle == 0) {
		AllocConsole();
		handle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	DWORD dwCharsWritten;
	WriteConsoleA(handle, str, (DWORD)strlen(str), &dwCharsWritten, NULL);
}

void DebugPrintAddress(unsigned __int64 addr) {
	const int BUF_LEN = 256;
	char buf[BUF_LEN];
	sprintf_s(buf, BUF_LEN, "%llx\n", addr);
	DebugPrint(buf);
}

bool shouldCap()
{
	return config.bSimpleMode == FALSE && (config.bLoadCapping || GetLoadingStatus() == FALSE);
}

microsecond_t Clock() {
	static LARGE_INTEGER frequency;
	static bool firstTime = true;

	if (firstTime) {
		timeBeginPeriod(1);
		QueryPerformanceFrequency(&frequency);
		firstTime = false;
	}

	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

	return (microsecond_t)(10e5 * time.QuadPart / frequency.QuadPart);
}

static microsecond_t prevClock = 0;
static microsecond_t currClock = 0;
static microsecond_t currFrameStartClock = 0;
static microsecond_t prevFrameStartClock = 0;
static microsecond_t fullFrameDelta = 0;
static unsigned __int64 frames = 0;

void Block(microsecond_t x) {
	microsecond_t t = Clock();
	while ((Clock() - t) < x) {}
}

void Cap(microsecond_t time) {
	microsecond_t bias = 500;
	millisecond_t sleepTimeInt = (millisecond_t)((time-bias) / 1000);
	sleepTimeInt = max(0, sleepTimeInt);
#ifdef PRECISE_CAP
	microsecond_t timeSlept = Clock();
	Sleep((DWORD)sleepTimeInt);
	timeSlept = Clock() - timeSlept;
	Block(max(0, time - timeSlept));
#else
	Sleep((DWORD)sleepTimeInt);
#endif
}

void ConfigAdjust() {
	if (config.bShowDiagnostics) {
		if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_UP)) {
			config.bSimpleMode = 0;
		}

		if (GetAsyncKeyState(VK_CONTROL) && GetAsyncKeyState(VK_DOWN)) {
			config.bSimpleMode = 1;
		}
	}
}

float ControllerSimple(float avgDeltaTime, float avgWorkTime, float avgFPS, float avgWPS, float v) {
	// Round the FPS to nearest integer
	float roundedFPS = roundf(avgFPS);
	float cv = 0.0f;
	
	if (roundedFPS >= config.fTargetFPS) {
		cv = config.fIncreaseRate;
	}

	if (roundedFPS < config.fTargetFPS) {
		cv = -config.fDecreaseRate;
	}

	if (roundedFPS < config.fEmergencyDropFPS) {
		cv = -config.fEmergencyDropRate;
	}

	return cv;
}

float ControllerAdvanced(float avgDeltaTime, float avgWorkTime, float avgFPS, float avgWPS,  float v) {
	// Round the FPS to nearest integer
	float roundedFPS = roundf(avgFPS);
	float cv = 0.0f;
	float strain = min(100.0f, 100.0f * (float)avgWorkTime / (float)avgDeltaTime);

	if (roundedFPS >= config.fTargetFPS && strain <= config.fIncreaseStrain) {
		cv = config.fIncreaseRate;
	}

	if (roundedFPS < config.fTargetFPS || strain >= config.fDecreaseStrain) {
		cv = -config.fDecreaseRate;
	}

	if (roundedFPS < config.fEmergencyDropFPS) {
		cv = -config.fEmergencyDropRate;
	}

	return cv;
}

float Controller(float avgDeltaTime, float avgWorkTime, float avgFPS, float avgWPS, float v) {
	if (config.bSimpleMode) {
		return ControllerSimple(avgDeltaTime, avgWorkTime, avgFPS, avgWPS, v);
	}
	return ControllerAdvanced(avgDeltaTime, avgWorkTime, avgFPS, avgWPS, v);
}

const char *NotString(bool value)
{
	return value ? " Is" : "Not";
}

void Tick() {
	const int NUM_DELTAS = 30;
	const int NUM_VIRTUAL_DELTAS = 20;
	const int CONTROLLER_UPDATE_INTERVAL = 5;

	float targetFPS = config.fTargetFPS;

	float valueMin = config.fShadowDirDistanceMin;
	float valueMax = config.fShadowDirDistanceMax;

	static microsecond_t sleepTime;
	static microsecond_t deltas[NUM_DELTAS] = { 0 };
	static microsecond_t workTimes[NUM_VIRTUAL_DELTAS] = { 0 };
	static bool freezeMode = false;

	// Measure the time between execution of Clock() again here
	prevClock = currClock;
	currClock = Clock();

	microsecond_t workTime = max(0, currClock - currFrameStartClock);
	deltas[frames%NUM_DELTAS] = fullFrameDelta;
	workTimes[frames%NUM_VIRTUAL_DELTAS] = workTime;

	float avgDelta = 0.0f;
	for (int i = 0; i < NUM_DELTAS; ++i) {
		avgDelta += (float)deltas[i];
	}

	float avgWorkTime = 0.0f;
	for (int i = 0; i < NUM_VIRTUAL_DELTAS; ++i) {
		avgWorkTime += (float)workTimes[i];
	}

	avgDelta /= (float)NUM_DELTAS;
	avgWorkTime /= (float)NUM_VIRTUAL_DELTAS;
	float strain = min(100.0f, 100.0f * (float)avgWorkTime / (float)avgDelta);
	float avgFPS = usecInSec / (float)avgDelta;
	float avgWPS = usecInSec / (float)avgWorkTime;

	if (frames % CONTROLLER_UPDATE_INTERVAL == 0) {
		ConfigAdjust();

		// Main variables
		float v = max(valueMin, GetShadowDirDistance());
		float cv = 0.0f;

		// Determine whether or not we should run the controller
		bool runController = true;
		if (config.bCheckPaused && GetGameUpdatePaused()) {
			runController = false;
		}
		if (config.bCheckPipboy && GetPipboyStatus()) {
			runController = false;
		}

		// Run the controller		
		if (runController) {
			cv = Controller(avgDelta, avgWorkTime, avgFPS, avgWPS, v);

			// Output smoothing via 5-frame moving average
			static const int SMOOTHED_LEN = 5;
			static int k = 0;
			static float smoothedSum = 0.0f;
			static float samples[SMOOTHED_LEN] = { 0.0f };
			samples[k%SMOOTHED_LEN] = cv;
			smoothedSum += cv;
			smoothedSum -= samples[(k + SMOOTHED_LEN + 1) % SMOOTHED_LEN];
			k++;
			cv = ((smoothedSum) / (float)(SMOOTHED_LEN - 1));

			// Apply the control variable
			v = v + cv;
			v = max(v, valueMin);
			v = min(v, valueMax);

			// Do the modification of shadows
			SetShadowDirDistance(v);
			SetShadowDistance(v);

			// Adjust volumetric lighting quality 
			if (config.bAdjustGRQuality != FALSE) {
				int grQuality = 0;
				if (v > config.fGRQualityShadowDist1)
					grQuality = 1;
				if (v > config.fGRQualityShadowDist2)
					grQuality = 2;
				if (v > config.fGRQualityShadowDist3)
					grQuality = 3;

				grQuality = max(grQuality, config.iGRQualityMin);
				grQuality = min(grQuality, config.iGRQualityMax);
				SetGRQuality(grQuality);
			}
		}

		// If debugging
		if (config.bShowDiagnostics) {
			const int BUF_LEN = 256;
			char buf[BUF_LEN];
			//                         1    2        3        4          5              6        7      8    9    10   11     12 
			sprintf_s(buf, BUF_LEN, "D%6d(%+4d), GR %1d, FPS %5.1f, WPS %5.1f, Strain %3.0f, FD %6lld, %8s, %7s, %5s, %4s, %6lldus\r",
				// 1							2			3			4		5		6		7				8									9										 10											11								  12
				(int)GetShadowDirDistance(), (int)cv, GetGRQuality(), avgFPS, avgWPS, strain, fullFrameDelta, freezeMode ? "Frozen" : "Changing", GetPipboyStatus() != 0 ? "Pip-Boy" : "", GetGameUpdatePaused() != 0 ? "Pause" : "", GetLoadingStatus() != 0 ? "Load" : "", sleepTime);
			DebugPrint(buf);
		}
	}

	microsecond_t targetDelta = (microsecond_t)((usecInSec) / targetFPS);
	static microsecond_t timeSlept = 0;
	microsecond_t workDelta = Clock() - currFrameStartClock;

	// Cap frame rate and record sleep time but only in advanced mode
	if (shouldCap()) {
		sleepTime = max(0, (targetDelta - workDelta));
		if (sleepTime) {
			Cap(sleepTime);
		}
	}
	else {
		sleepTime = -1;
	}

	// Increment the frame counter
	frames++;
}


void PrePresent() {
}

void PostPresent() {
	Tick();
	// We run after Present, so we'll treat this as the frame start
	prevFrameStartClock = currFrameStartClock;
	currFrameStartClock = Clock();
	fullFrameDelta = currFrameStartClock - prevFrameStartClock;
}

void InitConfig(config_t* const c) {
	c->fTargetFPS = 60.0f;
	c->fEmergencyDropFPS = 58.0f;
	c->fShadowDirDistanceMin = 2500.0f;
	c->fShadowDirDistanceMax = 12000.0f;
	c->fIncreaseRate = 100.0f;
	c->fDecreaseRate = 300.0f;
	c->fEmergencyDropRate = 500.0;
	c->bAdjustGRQuality = TRUE;
	c->iGRQualityMin = 0;
	c->iGRQualityMax = 3;
	c->bSimpleMode = FALSE;
	c->bShowDiagnostics = FALSE;
	c->fIncreaseStrain = 100.0f;
	c->fDecreaseStrain = 70.0f;
	c->fGRQualityShadowDist1 = 3000.0f;
	c->fGRQualityShadowDist2 = 6000.0f;
	c->fGRQualityShadowDist3 = 10000.0f;
	c->bCheckPipboy = TRUE;
	c->bCheckPaused = TRUE;
	c->bLoadCapping = FALSE;
}

void DebugPrintConfig() {
	const int BUF_LEN = 256;
	char buf[BUF_LEN];
	sprintf_s(buf, BUF_LEN, "fTargetFPS=%f\n", config.fTargetFPS);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fEmergencyDropFPS=%f\n", config.fEmergencyDropFPS);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fShadowDirDistanceMin=%f\n", config.fShadowDirDistanceMin);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fShadowDirDistanceMax=%f\n", config.fShadowDirDistanceMax);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fIncreaseRate=%f\n", config.fIncreaseRate);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fDecreaseRate=%f\n", config.fDecreaseRate);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fEmergencyDropRate=%f\n", config.fEmergencyDropRate);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "bAdjustGRQuality=%d\n", config.bAdjustGRQuality);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "bSimpleMode=%d\n", config.bSimpleMode);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fIncreaseStrain=%f\n", config.fIncreaseStrain);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fDecreaseStrain=%f\n", config.fDecreaseStrain);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fGRQualityShadowDist1=%f\n", config.fGRQualityShadowDist1);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fGRQualityShadowDist2=%f\n", config.fGRQualityShadowDist2);
	DebugPrint(buf);
	sprintf_s(buf, BUF_LEN, "fGRQualityShadowDist3=%f\n", config.fGRQualityShadowDist3);
	DebugPrint(buf);
}

void LoadConfig(const char* path, config_t* const c) {
	FILE* p = NULL;
	fopen_s(&p, path, "rt");
	if (p != NULL) {
		const int MAX_LINES = 96;
		const int MAX_LINE_CHARS = 256;
		fseek(p, 0, SEEK_SET);
		for (int i = 0; i < MAX_LINES; ++i) {
			char lineBuf[MAX_LINE_CHARS];
			fgets(lineBuf, MAX_LINE_CHARS, p);

			if (sscanf_s(lineBuf, "fTargetFPS=%f", &c->fTargetFPS) > 0) {
				c->fTargetFPS = max(0.0f, c->fTargetFPS);
			}

			if (sscanf_s(lineBuf, "fEmergencyDropFPS=%f", &c->fEmergencyDropFPS) > 0) {
				c->fEmergencyDropFPS = max(0.0f, c->fEmergencyDropFPS);
			}

			if (sscanf_s(lineBuf, "fIncreaseStrain=%f", &c->fIncreaseStrain) > 0) {
				c->fIncreaseStrain = max(0.0f, c->fIncreaseStrain);
			}

			if (sscanf_s(lineBuf, "fDecreaseStrain=%f", &c->fDecreaseStrain) > 0) {
				c->fDecreaseStrain = max(0.0f, c->fDecreaseStrain);
			}

			if (sscanf_s(lineBuf, "fShadowDirDistanceMin=%f", &c->fShadowDirDistanceMin) > 0) {
				c->fShadowDirDistanceMin = max(0.0f, c->fShadowDirDistanceMin);
			}

			if (sscanf_s(lineBuf, "fShadowDirDistanceMax=%f", &c->fShadowDirDistanceMax) > 0) {
				c->fShadowDirDistanceMax = max(0.0f, c->fShadowDirDistanceMax);
			}

			if (sscanf_s(lineBuf, "fIncreaseRate=%f", &c->fIncreaseRate) > 0) {
				c->fIncreaseRate = max(0.0f, c->fIncreaseRate);
			}

			if (sscanf_s(lineBuf, "fDecreaseRate=%f", &c->fDecreaseRate) > 0) {
				c->fDecreaseRate = max(0.0f, c->fDecreaseRate);
			}

			if (sscanf_s(lineBuf, "fEmergencyDropRate=%f", &c->fEmergencyDropRate) > 0) {
				c->fEmergencyDropRate = max(0.0f, c->fEmergencyDropRate);
			}

			if (sscanf_s(lineBuf, "fGRQualityShadowDist1=%f", &c->fGRQualityShadowDist1) > 0) {
				c->fGRQualityShadowDist1 = max(0.0f, c->fGRQualityShadowDist1);
			}

			if (sscanf_s(lineBuf, "fGRQualityShadowDist2=%f", &c->fGRQualityShadowDist2) > 0) {
				c->fGRQualityShadowDist2 = max(0.0f, c->fGRQualityShadowDist2);
			}

			if (sscanf_s(lineBuf, "fGRQualityShadowDist3=%f", &c->fGRQualityShadowDist3) > 0) {
				c->fGRQualityShadowDist3 = max(0.0f, c->fGRQualityShadowDist3);
			}

			if (sscanf_s(lineBuf, "bAdjustGRQuality=%d", &c->bAdjustGRQuality) > 0) {
				c->bAdjustGRQuality = (c->bAdjustGRQuality != 0) ? 1 : 0;
			}

			if (sscanf_s(lineBuf, "bSimpleMode=%d", &c->bSimpleMode) > 0) {
				c->bSimpleMode = (c->bSimpleMode != 0) ? 1 : 0;
			}

			if (sscanf_s(lineBuf, "bShowDiagnostics=%d", &c->bShowDiagnostics) > 0) {
				c->bShowDiagnostics = (c->bShowDiagnostics != 0) ? 1 : 0;
			}

			if (sscanf_s(lineBuf, "iGRQualityMin=%d", &c->iGRQualityMin) > 0) {
			}

			if (sscanf_s(lineBuf, "iGRQualityMax=%d", &c->iGRQualityMax) > 0) {
			}

			if (sscanf_s(lineBuf, "bLoadCapping=%d", &c->bLoadCapping) > 0) {
				c->bLoadCapping = (c->bLoadCapping != 0) ? 1 : 0;
			}
		}
		fclose(p);
	}
}