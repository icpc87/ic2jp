#include "Register.h"
#include "Globals.h"
#include <msctf.h>
#include <strsafe.h>
#include <string>

namespace {

constexpr wchar_t kImeDescription[]     = L"IC2JP Live Conversion IME";
constexpr wchar_t kProfileDescription[] = L"IC2JP ライブ変換";  // "IC2JP ライブ変換"

// Write a REG_SZ value; creates the key path if needed.
HRESULT SetRegValue(HKEY hRoot, const wchar_t* subKey,
                    const wchar_t* valueName, const wchar_t* data)
{
    HKEY hKey = nullptr;
    LONG lr = RegCreateKeyExW(hRoot, subKey, 0, nullptr,
                               REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr,
                               &hKey, nullptr);
    if (lr != ERROR_SUCCESS) return HRESULT_FROM_WIN32(lr);

    DWORD cbData = static_cast<DWORD>((wcslen(data) + 1) * sizeof(wchar_t));
    lr = RegSetValueExW(hKey, valueName, 0, REG_SZ,
                         reinterpret_cast<const BYTE*>(data), cbData);
    RegCloseKey(hKey);
    return HRESULT_FROM_WIN32(lr);
}

HRESULT DeleteRegKey(HKEY hRoot, const wchar_t* subKey)
{
    LONG lr = RegDeleteTreeW(hRoot, subKey);
    if (lr == ERROR_FILE_NOT_FOUND) return S_OK;
    return HRESULT_FROM_WIN32(lr);
}

std::wstring ClsidToString(REFCLSID clsid)
{
    wchar_t buf[64] = {};
    StringFromGUID2(clsid, buf, ARRAYSIZE(buf));
    return buf;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// RegisterServer
//
// 1. Writes HKLM\Software\Classes\CLSID\{...}\InprocServer32 COM key
// 2. Calls ITfInputProcessorProfiles::Register + AddLanguageProfile
//    so the IME appears in the language bar and Settings > Language
// ─────────────────────────────────────────────────────────────────────────────
HRESULT RegisterServer()
{
    wchar_t dllPath[MAX_PATH] = {};
    if (!GetModuleFileNameW(g_hInst, dllPath, MAX_PATH))
        return HRESULT_FROM_WIN32(GetLastError());

    const std::wstring clsidStr = ClsidToString(CLSID_IC2JP_IME);

    // COM InprocServer32 registration
    std::wstring clsidKey  = L"Software\\Classes\\CLSID\\" + clsidStr;
    std::wstring inprocKey = clsidKey + L"\\InprocServer32";

    HRESULT hr = SetRegValue(HKEY_LOCAL_MACHINE, clsidKey.c_str(),
                              nullptr, kImeDescription);
    if (FAILED(hr)) return hr;

    hr = SetRegValue(HKEY_LOCAL_MACHINE, inprocKey.c_str(), nullptr, dllPath);
    if (FAILED(hr)) return hr;

    hr = SetRegValue(HKEY_LOCAL_MACHINE, inprocKey.c_str(),
                      L"ThreadingModel", L"Apartment");
    if (FAILED(hr)) return hr;

    // TSF language profile registration
    ITfInputProcessorProfiles* pProfiles = nullptr;
    hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                           CLSCTX_INPROC_SERVER,
                           IID_ITfInputProcessorProfiles,
                           reinterpret_cast<void**>(&pProfiles));
    if (FAILED(hr)) return hr;

    hr = pProfiles->Register(CLSID_IC2JP_IME);
    if (SUCCEEDED(hr))
    {
        hr = pProfiles->AddLanguageProfile(
            CLSID_IC2JP_IME,
            IC2JP_LANGID,
            GUID_IC2JP_PROFILE,
            kProfileDescription,
            static_cast<ULONG>(wcslen(kProfileDescription)),
            dllPath,
            static_cast<ULONG>(wcslen(dllPath)),
            0 /* icon index */);
    }

    pProfiles->Release();
    return hr;
}

// ─────────────────────────────────────────────────────────────────────────────
// UnregisterServer
// ─────────────────────────────────────────────────────────────────────────────
HRESULT UnregisterServer()
{
    const std::wstring clsidStr = ClsidToString(CLSID_IC2JP_IME);
    DeleteRegKey(HKEY_LOCAL_MACHINE,
                  (L"Software\\Classes\\CLSID\\" + clsidStr).c_str());

    ITfInputProcessorProfiles* pProfiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfiles,
                                   reinterpret_cast<void**>(&pProfiles));
    if (FAILED(hr)) return hr;

    pProfiles->RemoveLanguageProfile(
        CLSID_IC2JP_IME, IC2JP_LANGID, GUID_IC2JP_PROFILE);
    pProfiles->Unregister(CLSID_IC2JP_IME);
    pProfiles->Release();
    return S_OK;
}
