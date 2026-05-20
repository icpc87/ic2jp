#pragma once
#include "Globals.h"
#include "DisplayAttribute.h"

// ITfDisplayAttributeProvider implementation.
//
// TSF calls EnumDisplayAttributeInfo to enumerate all attributes this text
// service owns, then calls GetDisplayAttributeInfo to retrieve each one.
// The provider is registered via ITfCategoryMgr::RegisterCategory with
// GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER.
//
// Implemented as a lightweight COM object that does NOT hold a reference
// to ImeCore — it is created once on Activate and released on Deactivate.
class DisplayAttributeProvider
    : public ITfDisplayAttributeProvider
{
public:
    DisplayAttributeProvider();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
    STDMETHOD_(ULONG, AddRef)()  override;
    STDMETHOD_(ULONG, Release)() override;

    // ITfDisplayAttributeProvider
    STDMETHOD(EnumDisplayAttributeInfo)(IEnumTfDisplayAttributeInfo** ppEnum) override;
    STDMETHOD(GetDisplayAttributeInfo)(REFGUID guid,
                                       ITfDisplayAttributeInfo** ppInfo,
                                       TfGuidAtom* pGuidAtom) override;

    // Called once on Activate to register with TSF.
    static HRESULT Register(TfClientId clientId, REFCLSID rclsid);
    static HRESULT Unregister(TfClientId clientId, REFCLSID rclsid);

private:
    ~DisplayAttributeProvider() = default;
    ULONG m_cRef;
};
