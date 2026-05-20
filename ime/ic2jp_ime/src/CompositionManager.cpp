#include "CompositionManager.h"
#include "EditSession.h"
#include "DisplayAttribute.h"

CompositionManager::CompositionManager(ITfThreadMgr* pThreadMgr,
                                        TfClientId    clientId)
    : m_pThreadMgr(pThreadMgr)
    , m_clientId(clientId)
    , m_pComposition(nullptr)
    , m_cRef(1)
    , m_mozc(std::make_unique<MozcClientStub>())
    , m_candidateWnd(std::make_unique<CandidateWindow>())
{
    m_candidateWnd->Create(g_hInst);
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
    m_hiragana.clear();
    m_romaji.Reset();
    m_mozc->Reset();
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
        if (!m_hiragana.empty() || !m_romaji.Pending().empty())
        {
            CommitComposition(pContext);
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    if (wParam == VK_ESCAPE)
    {
        if (!m_hiragana.empty() || !m_romaji.Pending().empty())
        {
            CancelComposition(pContext);
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    if (wParam == VK_BACK)
    {
        if (!m_romaji.Pending().empty())
        {
            // First backspace drains the romaji buffer before touching hiragana
            std::wstring pending = m_romaji.Pending();
            pending.pop_back();
            m_romaji.Reset();
            // Re-feed all but the last char
            for (wchar_t c : pending) m_romaji.Feed(c);
            _RefreshComposition(pContext);
            *pfEaten = TRUE;
        }
        else if (!m_hiragana.empty())
        {
            m_hiragana.pop_back();
            if (m_hiragana.empty())
                CancelComposition(pContext);
            else
                _RefreshComposition(pContext);
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    // Tab / Shift+Tab: cycle candidates
    if (wParam == VK_TAB)
    {
        if (!m_hiragana.empty())
        {
            bool shifted = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            auto result  = shifted ? m_mozc->SelectPrev() : m_mozc->SelectNext();
            if (!result.candidates.empty())
            {
                UpdateComposition(pContext, result.candidates[result.focused].value);
                *pfEaten = TRUE;
            }
        }
        return S_OK;
    }

    if (wParam >= 'A' && wParam <= 'Z')
    {
        bool shifted = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool caps    = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
        bool upper   = shifted ^ caps;
        wchar_t ch   = static_cast<wchar_t>(upper ? wParam : wParam + (L'a' - L'A'));

        // Feed into roman converter; accumulate resulting hiragana
        std::wstring kana = m_romaji.Feed(ch);
        m_hiragana += kana;

        _RefreshComposition(pContext);
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

// Run Mozc on current hiragana, update composition text, refresh candidate window.
HRESULT CompositionManager::_RefreshComposition(ITfContext* pContext)
{
    std::wstring display = m_hiragana + m_romaji.Pending();
    std::vector<std::wstring> candidateTexts;
    int focused = 0;

    if (!m_hiragana.empty())
    {
        auto result = m_mozc->Convert(m_hiragana);
        focused = result.focused;
        for (auto& c : result.candidates)
            candidateTexts.push_back(c.value);
        if (!result.candidates.empty())
            display = result.candidates[result.focused].value
                      + m_romaji.Pending();
    }

    // Update candidate window
    if (m_candidateWnd)
    {
        m_candidateWnd->Update(candidateTexts, focused);
        if (!candidateTexts.empty())
        {
            // Get caret screen position via GetCaretPos (approximate).
            // For precise positioning use ITfContextView::GetTextExt
            // inside an edit session on the composition range.
            POINT pt = {};
            GetCaretPos(&pt);
            ClientToScreen(GetFocus(), &pt);
            m_candidateWnd->Show(pt);
        }
        else
        {
            m_candidateWnd->Hide();
        }
    }

    return UpdateComposition(pContext, display);
}

// Open a READWRITE session to commit and end the composition.
HRESULT CompositionManager::CommitComposition(ITfContext* pContext)
{
    // Flush any unfinished romaji into hiragana first
    m_hiragana += m_romaji.Flush();

    if (!m_pComposition)
    {
        m_hiragana.clear();
        m_mozc->Reset();
        return S_OK;
    }

    std::wstring final = m_mozc->Commit();

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

    if (m_candidateWnd) m_candidateWnd->Hide();
    m_hiragana.clear();
    m_romaji.Reset();
    m_mozc->Reset();
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

    if (m_candidateWnd) m_candidateWnd->Hide();
    m_hiragana.clear();
    m_romaji.Reset();
    m_mozc->Reset();
    return hr;
}
