#include <sstream>
#include <cctype>
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

namespace
{
	typedef unsigned __int64 offset_t;

	bool FileExistsA(LPCSTR szPath)
	{
		DWORD dwAttrib = GetFileAttributesA(szPath);

		return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
			!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}


	HMODULE GetFalloutModuleHandle()
	{
		return GetModuleHandleA(NULL);
	}

	unsigned __int64 GetFalloutBaseAddress()
	{
		return (unsigned __int64)GetFalloutModuleHandle();
	}

	std::string getFileVersionNumberString(std::string filename)
	{
		auto cfilename = filename.c_str();
		auto size = GetFileVersionInfoSizeA(cfilename, nullptr);
		void *fileVersionInfo = malloc(size);
		VS_FIXEDFILEINFO *fixedFileInfo;
		GetFileVersionInfoA(cfilename, 0, size, fileVersionInfo);
		UINT fixedFileInfoSize;
		VerQueryValueA(fileVersionInfo, "\\", reinterpret_cast<LPVOID*>(&fixedFileInfo), &fixedFileInfoSize);
		std::stringstream str;
		str        << ((fixedFileInfo->dwFileVersionMS >> 0x10) & 0xffff)
			<< '.' << ((fixedFileInfo->dwFileVersionMS >> 0x00) & 0xffff)
			<< '.' << ((fixedFileInfo->dwFileVersionLS >> 0x10) & 0xffff)
			<< '.' << ((fixedFileInfo->dwFileVersionLS >> 0x00) & 0xffff);
		
		return str.str();
	}

	offset_t GetPrivateProfileHexOffset(LPCSTR lpAppName, LPCSTR lpKeyName, offset_t defaultValue, LPCSTR lpFileName)
	{
		char lpReturnedString[256];
		auto length = GetPrivateProfileStringA(lpAppName, lpKeyName, "", lpReturnedString, sizeof(lpReturnedString), lpFileName);
		if (length == 0)
		{
			return defaultValue;
		}
		offset_t result = 0;
		for (auto i = 0; i < length; ++i)
		{
			char c = std::toupper(lpReturnedString[i]);
			if (c >= '0' && c <= '9')
			{
				result = result * 0x10 + (c - '0');
			} else if (c >= 'A' && c <= 'F')
			{
				result = result * 0x10 + (c - 'A' + 10);
			} else
			{
				break;
			}
		}
		return result;
	}

}

char invalidAddress[16];

struct refTo
{
	unsigned __int64 base;
	unsigned __int64 address;

	template <typename type>
	operator type &()
	{
		if (address == -1)
		{
			return *reinterpret_cast<type *>(invalidAddress);
		}
		return *reinterpret_cast<type *>(base + address);
	}
};

struct Fallout4Addresses
{
	Fo4Float& shadowDirDistance;
	Fo4Int& volumetricQuality;
	const Fo4Int& gamePause;
	const Fo4Int& loadingStatus;
	const Fo4Int& mainMenuStatus;

	Fallout4Addresses(unsigned __int64 base, offset_t offset_fShadowDirDistance, offset_t offset_iVolumetricQuality,
	                  offset_t offset_bGameUpdatePaused, offset_t offset_bIsMainMenu, offset_t offset_bIsLoading
	) :
		shadowDirDistance(refTo{base, offset_fShadowDirDistance}),
		volumetricQuality(refTo{base, offset_iVolumetricQuality}),
		gamePause(refTo{base, offset_bGameUpdatePaused}),
		loadingStatus(refTo{base, offset_bIsLoading}),
		mainMenuStatus(refTo{base, offset_bIsMainMenu})
	{
	}
};

std::unique_ptr<Fallout4Addresses> make_fallout4_addresses(const std::string& fallout4_executable_filename)
{
	std::string addressIniFile = fallout4_executable_filename.substr(0, fallout4_executable_filename.find_last_of('\\')) + "\\fallout4-addresses-" + getFileVersionNumberString(fallout4_executable_filename) + ".ini";
	auto configFile = addressIniFile.c_str();
	if (!FileExistsA(configFile))
	{
		return std::unique_ptr<Fallout4Addresses>(nullptr);
	}
	
	const offset_t offset_fShadowDirDistance = GetPrivateProfileHexOffset("Addresses", "fShadowDirDistance", -1, configFile);
	const offset_t offset_iVolumetricQuality = GetPrivateProfileHexOffset("Addresses", "iVolumetricQuality", -1, configFile);
	const offset_t offset_bGameUpdatePaused = GetPrivateProfileHexOffset("Addresses", "bGameUpdatePaused", -1, configFile);
	const offset_t offset_bIsMainMenu = GetPrivateProfileHexOffset("Addresses", "bIsMainMenu", -1, configFile);
	const offset_t offset_bIsLoading = GetPrivateProfileHexOffset("Addresses", "bIsLoading", -1, configFile);

	return std::make_unique<Fallout4Addresses>(GetFalloutBaseAddress(), offset_fShadowDirDistance,
	                                           offset_iVolumetricQuality, offset_bGameUpdatePaused, offset_bIsMainMenu,
	                                           offset_bIsLoading);
}

Fallout4::Fallout4(const std::string fallout4ExecutableFilename) : addresses(
	make_fallout4_addresses(fallout4ExecutableFilename))
{
}

Fallout4::~Fallout4()
{
}


bool Fallout4::isGamePaused() const
{
	return addresses->gamePause != 0;
}

void Fallout4::setShadowDirDistance(float shadowDirDistance)
{
	addresses->shadowDirDistance = shadowDirDistance;
}

float Fallout4::getShadowDirDistance() const
{
	return addresses->shadowDirDistance;
}

void Fallout4::setVolumetricQuality(int volumetricQuality)
{
	addresses->volumetricQuality = volumetricQuality;
}

int Fallout4::getVolumetricQuality() const
{
	return addresses->volumetricQuality;
}

bool Fallout4::isGameLoading() const
{
	return addresses->loadingStatus != 0;
}

bool Fallout4::isMainMenu() const
{
	return addresses->mainMenuStatus != 0;
}
