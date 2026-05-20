#include "ImeCore.h"
#include "CompositionManager.h"

ImeCore::ImeCore()
    : m_cRef(1)
    , m_pThreadMgr(nullptr)
    , m_tfClientId(TF_CLIENTID_NULL)
{
    InterlockedIncrement(&g_cDllRef);
}

ImeCore::~ImeCore()
{
    InterlockedDecrement(&g_cDllRef);
}

// ─── IUnknown ────────────────────────────────────────────────────────────────

STDMETHODIMP ImeCore::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfTextInputProcessor))
    {
        *ppv = static_cast<ITfTextInputProcessor*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx))
    {
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
    {
        *ppv = static_cast<ITfKeyEventSink*>(this);
    }
    else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider))
    {
        *ppv = static_cast<ITfDisplayAttributeProvider*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) ImeCore::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) ImeCore::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

// ─── ITfTextInputProcessor ───────────────────────────────────────────────────

STDMETHODIMP ImeCore::Activate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId)
{
    // Compatibility shim — modern TSF calls ActivateEx directly.
    return ActivateEx(pThreadMgr, tfClientId, 0);
}

STDMETHODIMP ImeCore::Deactivate()
{
    return DoDeactivate();
}

// ─── ITfTextInputProcessorEx ─────────────────────────────────────────────────

STDMETHODIMP ImeCore::ActivateEx(ITfThreadMgr* pThreadMgr,
                                  TfClientId    tfClientId,
                                  DWORD         /*dwFlags*/)
{
    if (!pThreadMgr) return E_INVALIDARG;
    return DoActivate(pThreadMgr, tfClientId);
}

// ─── ITfKeyEventSink ─────────────────────────────────────────────────────────

STDMETHODIMP ImeCore::OnSetFocus(BOOL /*fForeground*/)
{
    return S_OK;
}

// OnTestKeyDown declares intent — return TRUE to claim the key before OnKeyDown.
STDMETHODIMP ImeCore::OnTestKeyDown(ITfContext* /*pContext*/,
                                     WPARAM wParam, LPARAM /*lParam*/,
                                     BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    // Claim printable alpha keys and backspace; everything else passes through.
    *pfEaten = (wParam >= 'A' && wParam <= 'Z') || (wParam == VK_BACK);
    return S_OK;
}

STDMETHODIMP ImeCore::OnTestKeyUp(ITfContext* /*pContext*/,
                                   WPARAM /*wParam*/, LPARAM /*lParam*/,
                                   BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP ImeCore::OnKeyDown(ITfContext* pContext,
                                 WPARAM wParam, LPARAM lParam,
                                 BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;

    if (!m_pCompositionMgr)
        return S_OK;

    return m_pCompositionMgr->HandleKeyDown(pContext, wParam, lParam, pfEaten);
}

STDMETHODIMP ImeCore::OnKeyUp(ITfContext* /*pContext*/,
                               WPARAM /*wParam*/, LPARAM /*lParam*/,
                               BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;
    return S_OK;
}

STDMETHODIMP ImeCore::OnPreservedKey(ITfContext* /*pContext*/,
                                      REFGUID /*rguid*/,
                                      BOOL* pfEaten)
{
    if (!pfEaten) return E_INVALIDARG;
    *pfEaten = FALSE;
    return S_OK;
}

// ─── Private helpers ─────────────────────────────────────────────────────────

HRESULT ImeCore::DoActivate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId)
{
    m_pThreadMgr = pThreadMgr;
    m_pThreadMgr->AddRef();
    m_tfClientId = tfClientId;

    // Advise ITfKeyEventSink so Windows routes key events here.
    ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
    HRESULT hr = m_pThreadMgr->QueryInterface(
        IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&pKeystrokeMgr));
    if (SUCCEEDED(hr))
    {
        hr = pKeystrokeMgr->AdviseKeyEventSink(
            m_tfClientId, static_cast<ITfKeyEventSink*>(this), TRUE);
        pKeystrokeMgr->Release();
    }

    if (FAILED(hr))
    {
        DoDeactivate();
        return hr;
    }

    m_pCompositionMgr =
        std::make_unique<CompositionManager>(m_pThreadMgr, m_tfClientId);

    // Register display attribute provider and category.
    m_pAttrProvider = std::make_unique<DisplayAttributeProvider>();
    DisplayAttributeProvider::Register(m_tfClientId, CLSID_IC2JP_IME);
    return S_OK;
}

HRESULT ImeCore::DoDeactivate()
{
    DisplayAttributeProvider::Unregister(m_tfClientId, CLSID_IC2JP_IME);
    m_pAttrProvider.reset();
    m_pCompositionMgr.reset();

    if (m_pThreadMgr && m_tfClientId != TF_CLIENTID_NULL)
    {
        ITfKeystrokeMgr* pKeystrokeMgr = nullptr;
        if (SUCCEEDED(m_pThreadMgr->QueryInterface(
                IID_ITfKeystrokeMgr, reinterpret_cast<void**>(&pKeystrokeMgr))))
        {
            pKeystrokeMgr->UnadviseKeyEventSink(m_tfClientId);
            pKeystrokeMgr->Release();
        }
    }

    if (m_pThreadMgr)
    {
        m_pThreadMgr->Release();
        m_pThreadMgr = nullptr;
    }
    m_tfClientId = TF_CLIENTID_NULL;
    return S_OK;
}

// ─── ITfDisplayAttributeProvider ─────────────────────────────────────────────

STDMETHODIMP ImeCore::EnumDisplayAttributeInfo(
    IEnumTfDisplayAttributeInfo** ppEnum)
{
    if (m_pAttrProvider)
        return m_pAttrProvider->EnumDisplayAttributeInfo(ppEnum);
    return E_FAIL;
}

STDMETHODIMP ImeCore::GetDisplayAttributeInfo(
    REFGUID guid, ITfDisplayAttributeInfo** ppInfo, TfGuidAtom* pGuidAtom)
{
    if (m_pAttrProvider)
        return m_pAttrProvider->GetDisplayAttributeInfo(guid, ppInfo, pGuidAtom);
    return E_FAIL;
}
