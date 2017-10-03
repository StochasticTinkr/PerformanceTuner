#include "Includes.h"
#include "Fallout.h"

namespace {
	const unsigned __int64 offset_fShadowDistance = 0x38e9778;
	const unsigned __int64 offset_fShadowDirDistance = 0x674fd0c;
//	const unsigned __int64 offset_fLookSensitivity = 0x37b8670;
	const unsigned __int64 offset_iVolumetricQuality = 0x38e8258;
	const unsigned __int64 offset_bPipboyStatus = 0x5af93b0;
	const unsigned __int64 offset_bGameUpdatePaused = 0x5a85340;
	const unsigned __int64 offset_bIsLoading = 0x5ada16c;

	HMODULE GetFalloutModuleHandle() {
		return GetModuleHandleA(NULL);
	}

	unsigned __int64 GetFalloutBaseAddress() {
		return (unsigned __int64)GetFalloutModuleHandle();
	}
}

struct refTo
{
	unsigned __int64 address;

	template <typename type>
	operator type &() {
		return *((type *)(address));
	}
};

struct Fallout4Addresses
{
	Fo4Float &shadowDistance;
	Fo4Float &shadowDirDistance;
	Fo4Int &volumetricQuality;
	const Fo4Int &pipboyStatus;
	const Fo4Int &gamePause;
	const Fo4Int &loadingStatus;

	Fallout4Addresses(unsigned __int64 base) :
		shadowDistance(refTo{ base + offset_fShadowDistance }),
		shadowDirDistance(refTo{ base + offset_fShadowDirDistance }),
		volumetricQuality(refTo{ base + offset_iVolumetricQuality }),
		pipboyStatus(refTo{ base + offset_bPipboyStatus }),
		gamePause(refTo{ base + offset_bGameUpdatePaused }),
		loadingStatus(refTo{ base + offset_bIsLoading })
	{}
};

Fallout4::Fallout4() : addresses(std::make_unique<Fallout4Addresses>(GetFalloutBaseAddress())) {
}

Fallout4::~Fallout4() {
}


bool Fallout4::isPipboyActive() const {
	return addresses->pipboyStatus != 0;
}

bool Fallout4::isGamePaused() const {
	return addresses->gamePause != 0;
}

void Fallout4::setShadowDistance(float shadowDistance) {
	addresses->shadowDistance = shadowDistance;
}

float Fallout4::getShadowDistance() const {
	return addresses->shadowDistance;
}

void Fallout4::setShadowDirDistance(float shadowDirDistance) {
	addresses->shadowDirDistance = shadowDirDistance;
}

float Fallout4::getShadowDirDistance() const {
	return addresses->shadowDirDistance;
}

void Fallout4::setVolumetricQuality(int volumetricQuality) {
	addresses->volumetricQuality = volumetricQuality;
}

int Fallout4::getVolumetricQuality() const {
	return addresses->volumetricQuality;
}

bool Fallout4::isGameLoading() const {
	return addresses->loadingStatus && addresses->gamePause;
}
