#pragma once
#include <windows.h>

// Called from DllRegisterServer / DllUnregisterServer.
// RegisterServer writes HKLM COM keys and registers the TSF language profile.
// UnregisterServer removes them via ITfInputProcessorProfiles::Unregister.
HRESULT RegisterServer();
HRESULT UnregisterServer();
