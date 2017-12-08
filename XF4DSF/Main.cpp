#include <dxgi.h>
#include <Psapi.h>

#include "Includes.h"
#include "Controller.h"
#include "Console.h"
#include "StringUtils.h"
#include "Fallout.h"

#pragma pack(1)

#define CONFIG_FILE "dynaperf.ini"
#define DXGI_DLL "dxgi.dll"
#define DXGI_LINKED_DLL "dxgi_linked.dll"

FARPROC p[9] = { 0 };
namespace
{
	std::unique_ptr<Controller> controller;

	void initController();

}

extern "C" BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	static HINSTANCE hL;
	if (reason == DLL_PROCESS_ATTACH) {
		// Load the original file from System32
		const int PATH_LEN = 512;
		TCHAR path[PATH_LEN];
		GetSystemDirectory(path, PATH_LEN);
		hL = LoadLibrary(lstrcat(path, _T("\\" DXGI_DLL)));

		if (!hL) return false;
		p[0] = GetProcAddress(hL, "CreateDXGIFactory");
		p[1] = GetProcAddress(hL, "CreateDXGIFactory1");
		p[2] = GetProcAddress(hL, "CreateDXGIFactory2");
		p[3] = GetProcAddress(hL, "DXGID3D10CreateDevice");
		p[4] = GetProcAddress(hL, "DXGID3D10CreateLayeredDevice");
		p[5] = GetProcAddress(hL, "DXGID3D10GetLayeredDeviceSize");
		p[6] = GetProcAddress(hL, "DXGID3D10RegisterLayers");
		p[7] = GetProcAddress(hL, "DXGIDumpJournal");
		p[8] = GetProcAddress(hL, "DXGIReportAdapterConfiguration");

		// Install the linked hooks if there
		LoadLibrary(_T(DXGI_LINKED_DLL));
		initController();
	}
	if (reason == DLL_PROCESS_DETACH) {
		FreeLibrary(hL);
		controller.release();
	}
	return TRUE;
}

typedef HRESULT  (__stdcall *CreateSwapChainPtr)(IDXGIFactory* pThis,
	_In_  IUnknown *pDevice, _In_  DXGI_SWAP_CHAIN_DESC *pDesc, _Out_  IDXGISwapChain **ppSwapChain);

typedef HRESULT(__stdcall *PresentPtr)(IDXGISwapChain* pThis,
	UINT, UINT);

CreateSwapChainPtr CreateSwapChain_original;
PresentPtr Present_original;

HRESULT __stdcall Hooked_IDXGISwapChain_Present(IDXGISwapChain* pThis, UINT a, UINT b) {
	//MessageBox(NULL, _T("Hooked_IDXGISwapChain_Present"), _T("dxgi.dll"), NULL);
	HRESULT hr = Present_original(pThis, a, b);
	if (controller.get()) {
		controller->tick();
	}
	return hr;
}

void Hook_IDXGISwapChain_Present(IDXGISwapChain* pBase) {
	const int idx = 8;
	unsigned __int64 *pVTableBase = (unsigned __int64 *)(*(unsigned __int64 *)pBase);
	unsigned __int64 *pVTableFnc = (unsigned __int64 *)((pVTableBase + idx));
	void *pHookFnc = (void *)Hooked_IDXGISwapChain_Present;

	Present_original = (PresentPtr)pVTableBase[idx];

	DWORD dwOldProtect = 0;
	(void)VirtualProtect(pVTableFnc, sizeof(void *), PAGE_EXECUTE_READWRITE, &dwOldProtect);
	memcpy(pVTableFnc, &pHookFnc, sizeof(void *));
	(void)VirtualProtect(pVTableFnc, sizeof(void *), dwOldProtect, &dwOldProtect);
}

HRESULT __stdcall Hooked_IDXGIFactory_CreateSwapChain(IDXGIFactory* pThis,
	_In_  IUnknown *pDevice, _In_  DXGI_SWAP_CHAIN_DESC *pDesc, _Out_  IDXGISwapChain **ppSwapChain) {
	// Call original
	HRESULT res = CreateSwapChain_original(pThis, pDevice, pDesc, ppSwapChain);
	Hook_IDXGISwapChain_Present(*ppSwapChain);
	return res;
}

void Hook_IDXGIFactory_CreateSwapChain(IDXGIFactory* pBase) {
	const int idx = 10;
	unsigned __int64 *pVTableBase = (unsigned __int64 *)(*(unsigned __int64 *)pBase);
	unsigned __int64 *pVTableFnc = (unsigned __int64 *)((pVTableBase + idx));
	void *pHookFnc = (void *)Hooked_IDXGIFactory_CreateSwapChain;

	CreateSwapChain_original = (CreateSwapChainPtr)pVTableBase[idx];

	DWORD dwOldProtect = 0;
	(void)VirtualProtect(pVTableFnc, sizeof(void *), PAGE_EXECUTE_READWRITE, &dwOldProtect);
	memcpy(pVTableFnc, &pHookFnc, sizeof(void *));
	(void)VirtualProtect(pVTableFnc, sizeof(void *), dwOldProtect, &dwOldProtect);
}

// ------------------------ Proxies ------------------------

extern "C" long Proxy_CreateDXGIFactory(__int64 a1, __int64 *a2) {
	typedef long(__stdcall *pS)(__int64 a1, __int64 *a2);
	pS pps = (pS)(p[0]);
	long rv = pps(a1, a2);
	// VMT hook the DXGIFactory
	IDXGIFactory* pFactory = (IDXGIFactory*)(*a2);
	Hook_IDXGIFactory_CreateSwapChain(pFactory);
	return rv;
}

extern "C" long Proxy_CreateDXGIFactory1(__int64 a1, __int64 *a2) {
	typedef long(__stdcall *pS)(__int64 a1, __int64 *a2);
	pS pps = (pS)(p[1]);
	long rv = pps(a1, a2);
	return rv;
}

extern "C" long Proxy_CreateDXGIFactory2(unsigned int  a1, __int64 a2, __int64 *a3) {
	typedef long(__stdcall *pS)(unsigned int  a1, __int64 a2, __int64 *a3);
	pS pps = (pS)(p[2]);
	long rv = pps(a1, a2, a3);
	return rv;
}

extern "C" long Proxy_DXGID3D10CreateDevice(HMODULE hModule, IDXGIFactory *pFactory, IDXGIAdapter *pAdapter,
	UINT Flags, void *unknown, void *ppDevice) {
	typedef long(__stdcall *pS)(HMODULE hModule, IDXGIFactory *pFactory, IDXGIAdapter *pAdapter,
		UINT Flags, void *unknown, void *ppDevice);
	pS pps = (pS)(p[3]);
	long rv = pps(hModule, pFactory, pAdapter, Flags, unknown, ppDevice);
	return rv;
}

struct UNKNOWN { BYTE unknown[20]; };
extern "C" long Proxy_DXGID3D10CreateLayeredDevice(UNKNOWN unknown) {
	typedef long(__stdcall *pS)(UNKNOWN);
	pS pps = (pS)(p[4]);
	long rv = pps(unknown);
	return rv;
}

extern "C" SIZE_T Proxy_DXGID3D10GetLayeredDeviceSize(const void *pLayers, UINT NumLayers) {
	typedef SIZE_T(__stdcall *pS)(const void *pLayers, UINT NumLayers);
	pS pps = (pS)(p[5]);
	SIZE_T rv = pps(pLayers,NumLayers);
	return rv;
}

extern "C" long Proxy_DXGID3D10RegisterLayers(const void *pLayers, UINT NumLayers) {
	typedef long(__stdcall *pS)(const void *pLayers, UINT NumLayers);
	pS pps = (pS)(p[6]);
	long rv = pps(pLayers,NumLayers);
	return rv;

}

extern "C" long Proxy_DXGIDumpJournal() {
	typedef long(__stdcall *pS)();
	pS pps = (pS)(p[7]);
	long rv = pps();
	return rv;
}

extern "C" long Proxy_DXGIReportAdapterConfiguration() {
	typedef long(__stdcall *pS)();
	pS pps = (pS)(p[8]);
	long rv = pps();
	return rv;
}

namespace
{
	std::string getCurrentFilename()
	{
		const int FILENAME_SIZE = 1024;
		CHAR cfilename[FILENAME_SIZE];
		return std::string(cfilename, GetModuleFileNameA(NULL, cfilename, FILENAME_SIZE));
	}
	
	void initController()
	{
		const std::string filename(getCurrentFilename());
		if (stringEndsWithIgnoreCase(filename, "\\Fallout4.exe")) {
			const std::string configFilename(replaceSuffix(filename, strlen("Fallout4.exe"), CONFIG_FILE));
			try
			{
				auto fallout4 = std::make_shared<Fallout4>(filename);
				controller = std::make_unique<Controller>(configFilename, fallout4);
			}
			catch (const std::exception& e)
			{
				console.print("Unable to start Dynamic Performance Tuner.\n");
				console.print(e.what());
				console.print("\n");
			}
		}
	}
}

