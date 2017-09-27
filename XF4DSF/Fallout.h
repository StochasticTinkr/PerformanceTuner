#ifndef __FALLOUT_H__
#define __FALLOUT_H__

#include "Includes.h"

const unsigned __int64 offset_fShadowDistance = 0x38e9778;
const unsigned __int64 offset_fShadowDirDistance = 0x674fd0c;
const unsigned __int64 offset_fLookSensitivity = 0x37b8670;
const unsigned __int64 offset_iVolumetricQuality = 0x38e8258;
const unsigned __int64 offset_bPipboyStatus = 0x5af93b0;
const unsigned __int64 offset_bGameUpdatePaused = 0x5a85340;
const unsigned __int64 offset_bIsLoading = 0x5ada16c;

void	InitAddresses();
HMODULE GetFalloutModuleHandle();

bool	GetPipboyStatus();
bool	GetGameUpdatePaused();
float	GetShadowDistance();
float	GetShadowDirDistance();
int		GetGRQuality();
int		GetLoadingStatus();

void	SetShadowDirDistance(float newValue);
void	SetShadowDistance(float newValue);
void	SetGRQuality(int newValue);

#endif // __FALLOUT_H__
