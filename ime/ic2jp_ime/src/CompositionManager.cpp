#include "CompositionManager.h"

CompositionManager::CompositionManager(ITfThreadMgr* pThreadMgr, TfClientId clientId)
    : m_pThreadMgr(pThreadMgr)
    , m_clientId(clientId)
    , m_pComposition(nullptr)
{
}

CompositionManager::~CompositionManager()
{
    // Deactivate should have ended the composition, but guard here.
    if (m_pComposition)
    {
        m_pComposition->EndComposition(nullptr);
        m_pComposition->Release();
        m_pComposition = nullptr;
    }
}

// ─── Public ──────────────────────────────────────────────────────────────────

HRESULT CompositionManager::HandleKeyDown(ITfContext* pContext,
                                           WPARAM      wParam,
                                           LPARAM      /*lParam*/,
                                           BOOL*       pfEaten)
{
    *pfEaten = FALSE;

    // Commit on Enter
    if (wParam == VK_RETURN)
    {
        if (!m_preedit.empty())
        {
            CommitComposition(pContext);
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    // Cancel on Escape
    if (wParam == VK_ESCAPE)
    {
        if (!m_preedit.empty())
        {
            CancelComposition(pContext);
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    // Backspace erases one preedit character
    if (wParam == VK_BACK)
    {
        if (!m_preedit.empty())
        {
            m_preedit.pop_back();
            if (m_preedit.empty())
                CancelComposition(pContext);
            else
                UpdateComposition(pContext, m_preedit);
            *pfEaten = TRUE;
        }
        return S_OK;
    }

    // Accept A-Z as roman-kana input (shift/caps handling omitted for now)
    if (wParam >= 'A' && wParam <= 'Z')
    {
        wchar_t ch = static_cast<wchar_t>(wParam + (L'a' - L'A'));
        m_preedit += ch;
        UpdateComposition(pContext, m_preedit);
        *pfEaten = TRUE;
        return S_OK;
    }

    return S_OK;
}

// ─── Private ─────────────────────────────────────────────────────────────────

HRESULT CompositionManager::StartComposition(ITfContext* pContext)
{
    if (m_pComposition)
        return S_OK;

    // Full implementation requires an ITfEditSession subclass.
    // Inside the edit session:
    //   1. Obtain ITfInsertAtSelection from pContext
    //   2. Call InsertTextAtSelection to get an anchor range
    //   3. Call ITfContextComposition::StartComposition with that range
    //   4. Store the returned ITfComposition* in m_pComposition
    //
    // Placeholder: structure is correct, body is a TODO stub.
    (void)pContext;
    return S_OK;
}

HRESULT CompositionManager::UpdateComposition(ITfContext* pContext,
                                               const std::wstring& preedit)
{
    HRESULT hr = StartComposition(pContext);
    if (FAILED(hr)) return hr;

    // Live conversion pipeline (to be wired to Mozc):
    //   1. Open a TF_ES_READWRITE edit session
    //   2. Call m_pComposition->GetRange to get the current range
    //   3. Pass m_preedit to Mozc session → get top candidate as m_converted
    //   4. Call ITfRange::SetText(nullptr, 0, m_converted.c_str(), len)
    //   5. Apply ITfProperty display attributes:
    //        - unconverted segments: TF_ATTR_INPUT (underline)
    //        - focused segment:      TF_ATTR_TARGET_CONVERTED (bold/highlight)
    //        - confirmed segments:   TF_ATTR_CONVERTED
    //
    // Placeholder: store string locally until edit-session plumbing is added.
    m_converted = preedit;
    (void)pContext;
    return S_OK;
}

HRESULT CompositionManager::CommitComposition(ITfContext* /*pContext*/)
{
    if (m_pComposition)
    {
        // TODO (inside edit session): SetText to m_converted before ending
        m_pComposition->EndComposition(nullptr);
        m_pComposition->Release();
        m_pComposition = nullptr;
    }
    m_preedit.clear();
    m_converted.clear();
    return S_OK;
}

HRESULT CompositionManager::CancelComposition(ITfContext* /*pContext*/)
{
    if (m_pComposition)
    {
        // TODO (inside edit session): clear text before ending
        m_pComposition->EndComposition(nullptr);
        m_pComposition->Release();
        m_pComposition = nullptr;
    }
    m_preedit.clear();
    m_converted.clear();
    return S_OK;
}
