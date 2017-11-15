#include "Includes.h"
#include "Fallout.h"
/**
 * <?xml version="1.0" encoding="utf-8"?>
<CheatTable>
  <CheatEntries>
    <CheatEntry>
      <ID>0</ID>
      <Description>"In main menu"</Description>
      <VariableType>4 Bytes</VariableType>
      <Address>Fallout4.exe+5B473A8</Address>
    </CheatEntry>
    <CheatEntry>
      <ID>5</ID>
      <Description>"Transitioning."</Description>
      <VariableType>4 Bytes</VariableType>
      <Address>Fallout4.exe+5A9D300</Address>
    </CheatEntry>
    <CheatEntry>
      <ID>7</ID>
      <Description>"Gameplay paused"</Description>
      <VariableType>4 Bytes</VariableType>
      <Address>Fallout4.exe+5AA50E4</Address>
    </CheatEntry>
  </CheatEntries>
</CheatTable>
<?xml version="1.0" encoding="utf-8"?>
<CheatTable>
<CheatEntries>
<CheatEntry>
<ID>1</ID>
<Description>"No description"</Description>
<LastState Value="15000" RealAddress="7FF6F5BBBA1C"/>
<VariableType>Float</VariableType>
<Address>Fallout4.exe+676BA1C</Address>
</CheatEntry>
</CheatEntries>
</CheatTable><?xml version="1.0" encoding="utf-8"?>
<CheatTable>
  <CheatEntries>
    <CheatEntry>
      <ID>2</ID>
      <Description>"No description"</Description>
      <LastState Value="1" RealAddress="7FF6F2D53FD8"/>
      <VariableType>4 Bytes</VariableType>
      <Address>Fallout4.exe+3903FD8</Address>
    </CheatEntry>
  </CheatEntries>
</CheatTable>


 */

namespace {
	const unsigned __int64 offset_fShadowDirDistance = 0x676BA1C;
	const unsigned __int64 offset_iVolumetricQuality = 0x3903FD8;
	const unsigned __int64 offset_bGameUpdatePaused = 0x5AA50E4;
	const unsigned __int64 offset_bIsMainMenu = 0x5B473A8;
	const unsigned __int64 offset_bIsLoading = 0x5A9D300;

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
	Fo4Float &shadowDirDistance;
	Fo4Int &volumetricQuality;
	const Fo4Int &gamePause;
	const Fo4Int &loadingStatus;
	const Fo4Int &mainMenuStatus;

	Fallout4Addresses(unsigned __int64 base) :
		shadowDirDistance(refTo{ base + offset_fShadowDirDistance }),
		volumetricQuality(refTo{ base + offset_iVolumetricQuality }),
		gamePause(refTo{ base + offset_bGameUpdatePaused }),
		loadingStatus(refTo{ base + offset_bIsLoading }),
		mainMenuStatus(refTo{ base + offset_bIsMainMenu })
	{}
};

Fallout4::Fallout4() : addresses(std::make_unique<Fallout4Addresses>(GetFalloutBaseAddress())) {
}

Fallout4::~Fallout4() {
}


bool Fallout4::isGamePaused() const {
	return addresses->gamePause != 0;
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
	return addresses->loadingStatus;
}

bool Fallout4::isMainMenu() const {
	return addresses->mainMenuStatus;
}
