#include "CompositionManager.h"
#include "EditSession.h"
#include "DisplayAttribute.h"

CompositionManager::CompositionManager(ITfThreadMgr* pThreadMgr,
                                        TfClientId    clientId)
    : m_pThreadMgr(pThreadMgr)
    , m_clientId(clientId)
    , m_pComposition(nullptr)
    , m_cRef(1)
{
}

CompositionManager::~CompositionManager()
{
    if (m_pComposition)
    {
        m_pComposition->EndComposition(nullptr);
        m_pComposition->Release();
        m_pComposition = nullptr;
    }
}

// ─── ITfCompositionSink ──────────────────────────────────────────────────────

STDMETHODIMP CompositionManager::OnCompositionTerminated(
    TfEditCookie /*ecWrite*/, ITfComposition* /*pComposition*/)
{
    // TSF ended the composition externally (e.g. focus change).
    if (m_pComposition)
    {
        m_pComposition->Release();
        m_pComposition = nullptr;
    }
    m_preedit.clear();
    return S_OK;
}

// ─── IUnknown ────────────────────────────────────────────────────────────────

STDMETHODIMP CompositionManager::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_ITfCompositionSink))
    {
        *ppv = static_cast<ITfCompositionSink*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CompositionManager::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CompositionManager::Release()
{
    return InterlockedDecrement(&m_cRef);
    // Not heap-allocated via COM; owned by ImeCore via unique_ptr.
}

// ─── Public ──────────────────────────────────────────────────────────────────

HRESULT CompositionManager::HandleKeyDown(ITfContext* pContext,
                                           WPARAM      wParam,
                                           LPARAM      /*lParam*/,
                                           BOOL*       pfEaten)
{
    *pfEaten = FALSE;

    if (wParam == VK_RETURN)
    {
        if (!m_preedit.empty())
        {
            CommitComposition(pContext);
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    if (wParam == VK_ESCAPE)
    {
        if (!m_preedit.empty())
        {
            CancelComposition(pContext);
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    if (wParam == VK_BACK)
    {
        if (!m_preedit.empty())
        {
            m_preedit.pop_back();
            if (m_preedit.empty())
                CancelComposition(pContext);
            else
                UpdateComposition(pContext, ConvertWithMozc(m_preedit));
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    if (wParam >= 'A' && wParam <= 'Z')
    {
        // Shift state: if Shift is held, keep uppercase for roman input
        bool shifted = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool caps    = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        bool upper   = shifted ^ caps;
        wchar_t ch   = static_cast<wchar_t>(upper ? wParam : wParam + (L'a' - L'A'));
        m_preedit += ch;
        UpdateComposition(pContext, ConvertWithMozc(m_preedit));
        *pfEaten = TRUE;
        return S_OK;
    }

    return S_OK;
}

// ─── Private ─────────────────────────────────────────────────────────────────

// Called from inside a READWRITE edit session to open the composition.
HRESULT CompositionManager::StartComposition(ITfContext* pContext,
                                              TfEditCookie ec)
{
    if (m_pComposition) return S_OK;

    // Step 1: Insert empty text at caret to get an anchor range.
    ITfInsertAtSelection* pInsert = nullptr;
    HRESULT hr = pContext->QueryInterface(IID_ITfInsertAtSelection,
                                          reinterpret_cast<void**>(&pInsert));
    if (FAILED(hr)) return hr;

    ITfRange* pRangeInsert = nullptr;
    hr = pInsert->InsertTextAtSelection(ec, TF_IAS_QUERYONLY,
                                         nullptr, 0, &pRangeInsert);
    pInsert->Release();
    if (FAILED(hr)) return hr;

    // Step 2: Start the composition on that range.
    ITfContextComposition* pCtxComp = nullptr;
    hr = pContext->QueryInterface(IID_ITfContextComposition,
                                   reinterpret_cast<void**>(&pCtxComp));
    if (SUCCEEDED(hr))
    {
        hr = pCtxComp->StartComposition(ec, pRangeInsert,
                                         static_cast<ITfCompositionSink*>(this),
                                         &m_pComposition);
        pCtxComp->Release();
    }
    pRangeInsert->Release();
    return hr;
}

// Open a READWRITE edit session to insert/update the composition string.
HRESULT CompositionManager::UpdateComposition(ITfContext*         pContext,
                                               const std::wstring& converted)
{
    // Capture by value so the lambda owns the string across the async callback.
    std::wstring text = converted;

    return EditSession::Run(pContext, m_clientId,
        TF_ES_READWRITE | TF_ES_SYNC,
        [this, &text, pContext](ITfEditSession*, TfEditCookie ec) -> HRESULT
        {
            HRESULT hr = StartComposition(pContext, ec);
            if (FAILED(hr) || !m_pComposition) return hr;

            // Get the range that covers the current composition string.
            ITfRange* pRange = nullptr;
            hr = m_pComposition->GetRange(&pRange);
            if (FAILED(hr)) return hr;

            // Overwrite the entire composition range with the converted text.
            hr = pRange->SetText(ec, TF_ST_CORRECTION,
                                  text.c_str(),
                                  static_cast<LONG>(text.size()));
            if (SUCCEEDED(hr))
                ApplyDisplayAttribute(pContext, ec, pRange, GUID_ATTR_INPUT);

            pRange->Release();
            return hr;
        });
}

// Apply a display attribute (underline / highlight) to a range via ITfProperty.
HRESULT CompositionManager::ApplyDisplayAttribute(ITfContext*  pContext,
                                                   TfEditCookie ec,
                                                   ITfRange*    pRange,
                                                   REFGUID      guidAttr)
{
    ITfProperty* pProp = nullptr;
    HRESULT hr = pContext->GetProperty(GUID_PROP_ATTRIBUTE, &pProp);
    if (FAILED(hr)) return hr;

    // The property value is a VT_I4 holding the atom for the attribute GUID.
    // TSF resolves the atom → TF_DISPLAYATTRIBUTE via ITfDisplayAttributeMgr.
    TfGuidAtom atom = TF_INVALID_GUIDATOM;
    ITfCategoryMgr* pCategoryMgr = nullptr;
    hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                           IID_ITfCategoryMgr,
                           reinterpret_cast<void**>(&pCategoryMgr));
    if (SUCCEEDED(hr))
    {
        hr = pCategoryMgr->RegisterGUID(guidAttr, &atom);
        pCategoryMgr->Release();
    }

    if (SUCCEEDED(hr) && atom != TF_INVALID_GUIDATOM)
    {
        VARIANT var;
        var.vt   = VT_I4;
        var.lVal = static_cast<LONG>(atom);
        hr = pProp->SetValue(ec, pRange, &var);
    }

    pProp->Release();
    return hr;
}

// Open a READWRITE session to commit and end the composition.
HRESULT CompositionManager::CommitComposition(ITfContext* pContext)
{
    if (!m_pComposition)
    {
        m_preedit.clear();
        return S_OK;
    }

    std::wstring final = ConvertWithMozc(m_preedit);

    HRESULT hr = EditSession::Run(pContext, m_clientId,
        TF_ES_READWRITE | TF_ES_SYNC,
        [this, &final](ITfEditSession*, TfEditCookie ec) -> HRESULT
        {
            if (!m_pComposition) return S_OK;

            ITfRange* pRange = nullptr;
            HRESULT hr2 = m_pComposition->GetRange(&pRange);
            if (SUCCEEDED(hr2))
            {
                // Write final committed text (no display attribute = plain text).
                hr2 = pRange->SetText(ec, TF_ST_CORRECTION,
                                       final.c_str(),
                                       static_cast<LONG>(final.size()));
                pRange->Release();
            }
            m_pComposition->EndComposition(ec);
            m_pComposition->Release();
            m_pComposition = nullptr;
            return hr2;
        });

    m_preedit.clear();
    return hr;
}

// Cancel: clear the composition range and end without inserting text.
HRESULT CompositionManager::CancelComposition(ITfContext* pContext)
{
    if (!m_pComposition)
    {
        m_preedit.clear();
        return S_OK;
    }

    HRESULT hr = EditSession::Run(pContext, m_clientId,
        TF_ES_READWRITE | TF_ES_SYNC,
        [this](ITfEditSession*, TfEditCookie ec) -> HRESULT
        {
            if (!m_pComposition) return S_OK;

            ITfRange* pRange = nullptr;
            if (SUCCEEDED(m_pComposition->GetRange(&pRange)))
            {
                pRange->SetText(ec, TF_ST_CORRECTION, L"", 0);
                pRange->Release();
            }
            m_pComposition->EndComposition(ec);
            m_pComposition->Release();
            m_pComposition = nullptr;
            return S_OK;
        });

    m_preedit.clear();
    return hr;
}

// ─── Mozc stub ───────────────────────────────────────────────────────────────
// Replace this with a real mozc::SessionInterface call:
//   mozc::commands::Input  input;
//   mozc::commands::Output output;
//   input.set_type(mozc::commands::Input::SEND_KEY);
//   input.mutable_key()->set_key_string(preedit);
//   session_->SendCommand(input, &output);
//   return output.preedit().segment(0).value();  // top candidate
std::wstring CompositionManager::ConvertWithMozc(const std::wstring& preedit)
{
    return preedit;  // passthrough until Mozc is linked
}
