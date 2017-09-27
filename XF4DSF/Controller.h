#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

// There are 10e5 microseconds in a second
typedef __int64 microsecond_t;
typedef __int64 millisecond_t;

struct config_t {
	int		bSimpleMode;
	int		bShowDiagnostics;
	int		bCheckPipboy;
	int		bCheckPaused;
	int		bLoadCapping;
	float	fTargetFPS;
	float	fEmergencyDropFPS;
	float	fShadowDirDistanceMin;
	float	fShadowDirDistanceMax;
	float	fIncreaseRate;
	float	fDecreaseRate;
	float	fEmergencyDropRate;
	float	fIncreaseStrain;
	float	fDecreaseStrain;
	int		bAdjustGRQuality;
	int		iGRQualityMin;
	int		iGRQualityMax;
	float	fGRQualityShadowDist1;
	float	fGRQualityShadowDist2;
	float	fGRQualityShadowDist3;
};

extern config_t config;

void InitConfig(config_t* const c);
void LoadConfig(const char* path, config_t* const c);
void DebugPrint(const char* str);
void DebugPrintConfig();
void PrePresent();
void PostPresent();

#endif // __CONTROLLER_H__
