#pragma once
#include "Globals.h"
#include <string>

// Owns the TSF composition (preedit) string and drives live conversion.
//
// Live-conversion loop (one keystroke):
//   HandleKeyDown → append to m_preedit → call Mozc (TODO) →
//   UpdateComposition (SetText on the composition range with the top candidate)
//
// The class is intentionally thin: edit sessions are opened inline for now.
// Factor them into separate ITfEditSession subclasses once the skeleton compiles.
class CompositionManager
{
public:
    CompositionManager(ITfThreadMgr* pThreadMgr, TfClientId clientId);
    ~CompositionManager();

    // Dispatch a WM_KEYDOWN equivalent from ImeCore::OnKeyDown.
    // Sets *pfEaten = TRUE when the keystroke is consumed by the IME.
    HRESULT HandleKeyDown(ITfContext* pContext,
                          WPARAM wParam, LPARAM lParam,
                          BOOL* pfEaten);

private:
    HRESULT StartComposition(ITfContext* pContext);
    HRESULT UpdateComposition(ITfContext* pContext, const std::wstring& preedit);
    HRESULT CommitComposition(ITfContext* pContext);
    HRESULT CancelComposition(ITfContext* pContext);

    ITfThreadMgr*  m_pThreadMgr;
    TfClientId     m_clientId;
    ITfComposition* m_pComposition;  // non-null while a composition is open
    std::wstring   m_preedit;        // roman/kana input accumulator
    std::wstring   m_converted;      // current Mozc top candidate (placeholder)
};
