#pragma once
#include "Globals.h"
#include <string>

// Owns the TSF composition (preedit) string and drives live conversion.
//
// Live-conversion loop (one keystroke):
//   HandleKeyDown → append to m_preedit → UpdateComposition →
//     EditSession(READWRITE) → SetText(converted) + SetDisplayAttribute
//
// Mozc integration point: replace ConvertWithMozc() stub with a real
// mozc::SessionInterface call once the Mozc client is linked.
class CompositionManager : public ITfCompositionSink
{
public:
    CompositionManager(ITfThreadMgr* pThreadMgr, TfClientId clientId);
    ~CompositionManager();

    // ITfCompositionSink — called by TSF when the composition is terminated
    // externally (e.g. the user clicks somewhere else).
    STDMETHOD(OnCompositionTerminated)(TfEditCookie ecWrite,
                                       ITfComposition* pComposition) override;

    // IUnknown (minimal — CompositionManager is not reference-counted by COM)
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
    STDMETHOD_(ULONG, AddRef)()  override;
    STDMETHOD_(ULONG, Release)() override;

    // Dispatch a WM_KEYDOWN equivalent from ImeCore::OnKeyDown.
    // Sets *pfEaten = TRUE when the keystroke is consumed by the IME.
    HRESULT HandleKeyDown(ITfContext* pContext,
                          WPARAM wParam, LPARAM lParam,
                          BOOL* pfEaten);

private:
    HRESULT StartComposition(ITfContext* pContext, TfEditCookie ec);
    HRESULT UpdateComposition(ITfContext* pContext,
                               const std::wstring& converted);
    HRESULT ApplyDisplayAttribute(ITfContext* pContext, TfEditCookie ec,
                                   ITfRange* pRange, REFGUID guidAttr);
    HRESULT CommitComposition(ITfContext* pContext);
    HRESULT CancelComposition(ITfContext* pContext);

    // Mozc conversion stub — returns preedit as-is until Mozc is linked.
    std::wstring ConvertWithMozc(const std::wstring& preedit);

    ITfThreadMgr*   m_pThreadMgr;
    TfClientId      m_clientId;
    ITfComposition* m_pComposition;  // non-null while a composition is open
    ULONG           m_cRef;
    std::wstring    m_preedit;        // roman/kana input accumulator
};
