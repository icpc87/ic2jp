#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <msctf.h>
#include <new>

// ─────────────────────────────────────────────────────────────────────────────
// GUIDs — replace ALL THREE with fresh values from:
//   Visual Studio: Tools > Create GUID (DEFINE_GUID format → initializer form)
//   Command line:  uuidgen.exe
// ─────────────────────────────────────────────────────────────────────────────

// CLSID of the COM text service object
// {A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
extern const CLSID CLSID_IC2JP_IME;

// GUID identifying this IME's Japanese language profile
// {B2C3D4E5-F6A7-8901-BCDE-F12345678901}
extern const GUID GUID_IC2JP_PROFILE;

// ─────────────────────────────────────────────────────────────────────────────
// Language constant  (Japanese: 0x0411)
// ─────────────────────────────────────────────────────────────────────────────
constexpr LANGID IC2JP_LANGID = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);

// ─────────────────────────────────────────────────────────────────────────────
// DLL-level globals shared across translation units
// ─────────────────────────────────────────────────────────────────────────────
extern HINSTANCE g_hInst;
extern ULONG     g_cDllRef;  // aggregate object count, guards DllCanUnloadNow
