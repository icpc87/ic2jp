#include "EditSession.h"

// ─── Static helper ────────────────────────────────────────────────────────────

HRESULT EditSession::Run(ITfContext* pContext, TfClientId clientId,
                         DWORD flags, Callback cb)
{
    if (!pContext) return E_INVALIDARG;

    EditSession* pSession = new(std::nothrow) EditSession(std::move(cb));
    if (!pSession) return E_OUTOFMEMORY;

    HRESULT hrSession = S_OK;
    HRESULT hr = pContext->RequestEditSession(clientId, pSession,
                                               flags, &hrSession);
    pSession->Release();

    return SUCCEEDED(hr) ? hrSession : hr;
}

// ─── IUnknown ────────────────────────────────────────────────────────────────

STDMETHODIMP EditSession::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfEditSession))
    {
        *ppv = static_cast<ITfEditSession*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) EditSession::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) EditSession::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) delete this;
    return cRef;
}

// ─── ITfEditSession ──────────────────────────────────────────────────────────

STDMETHODIMP EditSession::DoEditSession(TfEditCookie ec)
{
    // Pass ec via a thin wrapper so callers don't need to subclass.
    // We repurpose the first parameter (ITfEditSession*) as a typed self-ref;
    // the lambda only needs ec.
    m_hrResult = m_cb(this, ec);
    return m_hrResult;
}
