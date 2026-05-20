#include "Globals.h"
#include "ClassFactory.h"
#include "Register.h"

// ─── GUID definitions ────────────────────────────────────────────────────────
// !!! Replace these with fresh GUIDs from guidgen.exe before shipping !!!

// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
const CLSID CLSID_IC2JP_IME =
    { 0xa1b2c3d4, 0xe5f6, 0x7890,
      { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 } };

// {B2C3D4E5-F6A7-8901-BCDE-F12345678901}
const GUID GUID_IC2JP_PROFILE =
    { 0xb2c3d4e5, 0xf6a7, 0x8901,
      { 0xbc, 0xde, 0xf1, 0x23, 0x45, 0x67, 0x89, 0x01 } };

// ─── DLL globals ─────────────────────────────────────────────────────────────
HINSTANCE g_hInst   = nullptr;
ULONG     g_cDllRef = 0;

// ─── DLL entry point ─────────────────────────────────────────────────────────
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstance;
        DisableThreadLibraryCalls(hInstance);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// ─── COM infrastructure exports (forwarded from .def) ────────────────────────

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if (!ppv)
        return E_INVALIDARG;
    *ppv = nullptr;

    if (!IsEqualCLSID(rclsid, CLSID_IC2JP_IME))
        return CLASS_E_CLASSNOTAVAILABLE;

    ClassFactory* pFactory = new(std::nothrow) ClassFactory();
    if (!pFactory)
        return E_OUTOFMEMORY;

    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();
    return hr;
}

STDAPI DllCanUnloadNow()
{
    return (g_cDllRef == 0) ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer()
{
    return RegisterServer();
}

STDAPI DllUnregisterServer()
{
    return UnregisterServer();
}
