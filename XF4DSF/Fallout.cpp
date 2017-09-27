#include "Fallout.h"

static float* addr_fShadowDistance = 0;
static float* addr_fShadowDirDistance = 0;
static float* addr_fLookSensitivity = 0;
static __int32* addr_iVolumetricQuality = 0;
static __int32* addr_bPipboyStatus = 0;
static __int32* addr_bGameUpdatePaused = 0;
static __int32* addr_bIsLoading = 0;

HMODULE GetFalloutModuleHandle() {
	return GetModuleHandleA(NULL);
}

unsigned __int64 GetFalloutBaseAddress() {
	return (unsigned __int64)GetFalloutModuleHandle();
}

void InitAddresses() {
	unsigned __int64 base = GetFalloutBaseAddress();
	addr_fShadowDistance = (float*)(base + offset_fShadowDistance);
	addr_fShadowDirDistance = (float*)(base + offset_fShadowDirDistance);
	addr_fLookSensitivity = (float*)(base + offset_fLookSensitivity);
	addr_iVolumetricQuality = (__int32*)(base + offset_iVolumetricQuality);
	addr_bPipboyStatus = (__int32*)(base + offset_bPipboyStatus);
	addr_bGameUpdatePaused = (__int32*)(base + offset_bGameUpdatePaused);
	addr_bIsLoading = (__int32*)(base + offset_bIsLoading);
}

bool GetPipboyStatus() {
	return (*addr_bPipboyStatus);
}

bool GetGameUpdatePaused() {
	return (*addr_bGameUpdatePaused);
}

void SetShadowDistance(float newValue) {
	(*addr_fShadowDistance) = newValue;
}

float GetShadowDistance() {
	return *addr_fShadowDistance;
}

void SetShadowDirDistance(float newValue) {
	(*addr_fShadowDirDistance) = newValue;
}

float GetShadowDirDistance() {
	return *addr_fShadowDirDistance;
}

void SetLookSensitivity(float newValue) {
	(*addr_fLookSensitivity) = newValue;
}

void SetGRQuality(int newValue) {
	(*addr_iVolumetricQuality) = newValue;
}

int GetGRQuality() {
	return (*addr_iVolumetricQuality);
}

int	GetLoadingStatus() {
	return (*addr_bIsLoading) && (*addr_bGameUpdatePaused);
}
