#include "DisplayAttribute.h"

// !!! Replace with fresh GUIDs from guidgen.exe !!!

// {C3D4E5F6-A7B8-9012-CDEF-123456789012}
const GUID GUID_ATTR_INPUT =
    { 0xc3d4e5f6, 0xa7b8, 0x9012,
      { 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12 } };

// {D4E5F6A7-B8C9-0123-DEFA-234567890123}
const GUID GUID_ATTR_TARGET_CONVERTED =
    { 0xd4e5f6a7, 0xb8c9, 0x0123,
      { 0xde, 0xfa, 0x23, 0x45, 0x67, 0x89, 0x01, 0x23 } };

HRESULT GetDisplayAttribute(REFGUID guidAttr, TF_DISPLAYATTRIBUTE* pda)
{
    if (!pda) return E_INVALIDARG;
    ZeroMemory(pda, sizeof(*pda));

    if (IsEqualGUID(guidAttr, GUID_ATTR_INPUT))
    {
        // Preedit (unconverted input): thin underline, default colors
        pda->lsStyle          = TF_LS_DOT;
        pda->fBoldLine        = FALSE;
        pda->crText.type      = TF_CT_NONE;
        pda->crBk.type        = TF_CT_NONE;
        pda->crLine.type      = TF_CT_NONE;
        pda->bAttr            = TF_ATTR_INPUT;
        return S_OK;
    }

    if (IsEqualGUID(guidAttr, GUID_ATTR_TARGET_CONVERTED))
    {
        // Focused candidate segment: solid underline, highlight background
        pda->lsStyle          = TF_LS_SOLID;
        pda->fBoldLine        = TRUE;
        pda->crText.type      = TF_CT_NONE;
        pda->crBk.type        = TF_CT_SYSCOLOR;
        pda->crBk.nIndex      = COLOR_HIGHLIGHT;
        pda->crLine.type      = TF_CT_NONE;
        pda->bAttr            = TF_ATTR_TARGET_CONVERTED;
        return S_OK;
    }

    return E_INVALIDARG;
}
