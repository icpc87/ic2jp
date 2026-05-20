#pragma once
#include "Globals.h"
#include <functional>

// Generic synchronous edit session.
//
// TSF requires all document writes to happen inside DoEditSession callbacks.
// This helper wraps a lambda so callers don't need a new subclass per operation.
//
// Usage:
//   EditSession::Run(pContext, m_clientId, TF_ES_READWRITE | TF_ES_SYNC,
//       [&](ITfEditSession*, TfEditCookie ec) -> HRESULT {
//           return pRange->SetText(ec, 0, text.c_str(), (LONG)text.size());
//       });
class EditSession : public ITfEditSession
{
public:
    using Callback = std::function<HRESULT(ITfEditSession*, TfEditCookie)>;

    // Open an edit session on pContext and invoke cb inside it.
    // flags: TF_ES_SYNC | TF_ES_READWRITE  (or TF_ES_READ for read-only)
    static HRESULT Run(ITfContext* pContext, TfClientId clientId,
                       DWORD flags, Callback cb);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
    STDMETHOD_(ULONG, AddRef)()  override;
    STDMETHOD_(ULONG, Release)() override;

    // ITfEditSession
    STDMETHOD(DoEditSession)(TfEditCookie ec) override;

private:
    explicit EditSession(Callback cb) : m_cRef(1), m_cb(std::move(cb)) {}
    ~EditSession() = default;

    ULONG    m_cRef;
    Callback m_cb;
    HRESULT  m_hrResult = S_OK;
};
