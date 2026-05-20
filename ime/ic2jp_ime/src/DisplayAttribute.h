#pragma once
#include "Globals.h"

// GUIDs for the two display attribute properties we advertise.
// Replace with fresh values from guidgen.exe.
//
// {C3D4E5F6-A7B8-9012-CDEF-123456789012}  — input (preedit underline)
extern const GUID GUID_ATTR_INPUT;
// {D4E5F6A7-B8C9-0123-DEFA-234567890123}  — target converted (focused segment)
extern const GUID GUID_ATTR_TARGET_CONVERTED;

// Fill a TF_DISPLAYATTRIBUTE struct for the given attribute GUID.
// Returns E_INVALIDARG if the GUID is not one we own.
HRESULT GetDisplayAttribute(REFGUID guidAttr, TF_DISPLAYATTRIBUTE* pda);
