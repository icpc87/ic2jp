#include "DisplayAttributeProvider.h"

// ─── Simple per-attribute COM object ─────────────────────────────────────────

// TSF needs each attribute wrapped in an ITfDisplayAttributeInfo object.
class DisplayAttributeInfo : public ITfDisplayAttributeInfo
{
public:
    DisplayAttributeInfo(REFGUID guid, const wchar_t* desc)
        : m_cRef(1), m_guid(guid), m_desc(desc) {}

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_ITfDisplayAttributeInfo))
        {
            *ppv = static_cast<ITfDisplayAttributeInfo*>(this);
            AddRef(); return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)()  override { return InterlockedIncrement(&m_cRef); }
    STDMETHOD_(ULONG, Release)() override
    {
        ULONG r = InterlockedDecrement(&m_cRef);
        if (r == 0) delete this;
        return r;
    }

    // ITfDisplayAttributeInfo
    STDMETHOD(GetGUID)(GUID* pGuid) override
    {
        if (!pGuid) return E_INVALIDARG;
        *pGuid = m_guid; return S_OK;
    }
    STDMETHOD(GetDescription)(BSTR* pbstr) override
    {
        if (!pbstr) return E_INVALIDARG;
        *pbstr = SysAllocString(m_desc);
        return *pbstr ? S_OK : E_OUTOFMEMORY;
    }
    STDMETHOD(GetAttributeInfo)(TF_DISPLAYATTRIBUTE* pda) override
    {
        return GetDisplayAttribute(m_guid, pda);
    }
    STDMETHOD(SetAttributeInfo)(const TF_DISPLAYATTRIBUTE* /*pda*/) override
    {
        return E_NOTIMPL;  // read-only
    }
    STDMETHOD(Reset)() override { return S_OK; }

private:
    ~DisplayAttributeInfo() = default;
    ULONG        m_cRef;
    GUID         m_guid;
    const wchar_t* m_desc;
};

// ─── Simple enumerator ────────────────────────────────────────────────────────

class EnumDisplayAttributeInfo : public IEnumTfDisplayAttributeInfo
{
public:
    EnumDisplayAttributeInfo() : m_cRef(1), m_index(0) {}

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override
    {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo))
        {
            *ppv = static_cast<IEnumTfDisplayAttributeInfo*>(this);
            AddRef(); return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)()  override { return InterlockedIncrement(&m_cRef); }
    STDMETHOD_(ULONG, Release)() override
    {
        ULONG r = InterlockedDecrement(&m_cRef);
        if (r == 0) delete this;
        return r;
    }

    STDMETHOD(Next)(ULONG count, ITfDisplayAttributeInfo** ppInfo,
                    ULONG* pFetched) override
    {
        static const struct { const GUID* guid; const wchar_t* desc; } kAttrs[] = {
            { &GUID_ATTR_INPUT,            L"IC2JP Input"            },
            { &GUID_ATTR_TARGET_CONVERTED, L"IC2JP Target Converted" },
        };
        ULONG fetched = 0;
        while (fetched < count && m_index < ARRAYSIZE(kAttrs))
        {
            ppInfo[fetched] = new(std::nothrow)
                DisplayAttributeInfo(*kAttrs[m_index].guid, kAttrs[m_index].desc);
            if (!ppInfo[fetched]) break;
            ++fetched; ++m_index;
        }
        if (pFetched) *pFetched = fetched;
        return fetched == count ? S_OK : S_FALSE;
    }
    STDMETHOD(Skip)(ULONG count) override
    {
        m_index = std::min(m_index + count, 2u);
        return S_OK;
    }
    STDMETHOD(Reset)() override { m_index = 0; return S_OK; }
    STDMETHOD(Clone)(IEnumTfDisplayAttributeInfo** ppEnum) override
    {
        if (!ppEnum) return E_INVALIDARG;
        auto* p = new(std::nothrow) EnumDisplayAttributeInfo();
        if (!p) return E_OUTOFMEMORY;
        p->m_index = m_index;
        *ppEnum = p;
        return S_OK;
    }

private:
    ~EnumDisplayAttributeInfo() = default;
    ULONG m_cRef;
    ULONG m_index;
};

// ─── DisplayAttributeProvider ────────────────────────────────────────────────

DisplayAttributeProvider::DisplayAttributeProvider() : m_cRef(1) {}

STDMETHODIMP DisplayAttributeProvider::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
    {
        *ppv = static_cast<ITfDisplayAttributeProvider*>(this);
        AddRef(); return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) DisplayAttributeProvider::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) DisplayAttributeProvider::Release()
{
    ULONG r = InterlockedDecrement(&m_cRef);
    if (r == 0) delete this;
    return r;
}

STDMETHODIMP DisplayAttributeProvider::EnumDisplayAttributeInfo(
    IEnumTfDisplayAttributeInfo** ppEnum)
{
    if (!ppEnum) return E_INVALIDARG;
    *ppEnum = new(std::nothrow) EnumDisplayAttributeInfo();
    return *ppEnum ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP DisplayAttributeProvider::GetDisplayAttributeInfo(
    REFGUID guid, ITfDisplayAttributeInfo** ppInfo, TfGuidAtom* pGuidAtom)
{
    if (!ppInfo) return E_INVALIDARG;
    *ppInfo = nullptr;
    if (pGuidAtom) *pGuidAtom = TF_INVALID_GUIDATOM;

    const wchar_t* desc = nullptr;
    if (IsEqualGUID(guid, GUID_ATTR_INPUT))
        desc = L"IC2JP Input";
    else if (IsEqualGUID(guid, GUID_ATTR_TARGET_CONVERTED))
        desc = L"IC2JP Target Converted";
    else
        return E_INVALIDARG;

    *ppInfo = new(std::nothrow) DisplayAttributeInfo(guid, desc);
    if (!*ppInfo) return E_OUTOFMEMORY;

    if (pGuidAtom)
    {
        ITfCategoryMgr* pCat = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                                        CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                        reinterpret_cast<void**>(&pCat))))
        {
            pCat->RegisterGUID(guid, pGuidAtom);
            pCat->Release();
        }
    }
    return S_OK;
}

// ─── Registration helpers ─────────────────────────────────────────────────────

HRESULT DisplayAttributeProvider::Register(TfClientId clientId, REFCLSID rclsid)
{
    ITfCategoryMgr* pCat = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                                   CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                   reinterpret_cast<void**>(&pCat));
    if (FAILED(hr)) return hr;

    // Register ourselves as a display attribute provider.
    hr = pCat->RegisterCategory(rclsid,
                                  GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, rclsid);
    if (SUCCEEDED(hr))
        hr = pCat->RegisterCategory(rclsid,
                                      GUID_TFCAT_TIP_KEYBOARD, rclsid);
    pCat->Release();
    (void)clientId;
    return hr;
}

HRESULT DisplayAttributeProvider::Unregister(TfClientId /*clientId*/,
                                              REFCLSID rclsid)
{
    ITfCategoryMgr* pCat = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr,
                                   CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr,
                                   reinterpret_cast<void**>(&pCat));
    if (FAILED(hr)) return hr;

    pCat->UnregisterCategory(rclsid, GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER, rclsid);
    pCat->UnregisterCategory(rclsid, GUID_TFCAT_TIP_KEYBOARD, rclsid);
    pCat->Release();
    return S_OK;
}
