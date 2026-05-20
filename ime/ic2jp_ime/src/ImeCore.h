#pragma once
#include "Globals.h"
#include "DisplayAttributeProvider.h"
#include <memory>

class CompositionManager;

// Central text service class.
//
// Interface map:
//   IUnknown
//   ITfTextInputProcessor         — legacy activation (delegates to ActivateEx)
//   ITfTextInputProcessorEx       — preferred activation
//   ITfKeyEventSink               — advised on Activate, unadvised on Deactivate
//   ITfDisplayAttributeProvider   — supplies underline/highlight attributes
//
// Threading: TSF calls all methods on the UI thread (apartment-threaded COM).
class ImeCore
    : public ITfTextInputProcessorEx
    , public ITfKeyEventSink
    , public ITfDisplayAttributeProvider
{
public:
    ImeCore();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
    STDMETHOD_(ULONG, AddRef)()  override;
    STDMETHOD_(ULONG, Release)() override;

    // ITfTextInputProcessor
    STDMETHOD(Activate)(ITfThreadMgr* pThreadMgr, TfClientId tfClientId) override;
    STDMETHOD(Deactivate)() override;

    // ITfTextInputProcessorEx
    STDMETHOD(ActivateEx)(ITfThreadMgr* pThreadMgr, TfClientId tfClientId,
                          DWORD dwFlags) override;

    // ITfKeyEventSink
    STDMETHOD(OnSetFocus)(BOOL fForeground) override;
    STDMETHOD(OnTestKeyDown)(ITfContext* pContext,
                             WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHOD(OnTestKeyUp)  (ITfContext* pContext,
                             WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHOD(OnKeyDown)    (ITfContext* pContext,
                             WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHOD(OnKeyUp)      (ITfContext* pContext,
                             WPARAM wParam, LPARAM lParam, BOOL* pfEaten) override;
    STDMETHOD(OnPreservedKey)(ITfContext* pContext,
                              REFGUID rguid, BOOL* pfEaten) override;

    // ITfDisplayAttributeProvider
    STDMETHOD(EnumDisplayAttributeInfo)(IEnumTfDisplayAttributeInfo** ppEnum) override;
    STDMETHOD(GetDisplayAttributeInfo)(REFGUID guid,
                                       ITfDisplayAttributeInfo** ppInfo,
                                       TfGuidAtom* pGuidAtom) override;

private:
    ~ImeCore();

    HRESULT DoActivate(ITfThreadMgr* pThreadMgr, TfClientId tfClientId);
    HRESULT DoDeactivate();

    ULONG         m_cRef;
    ITfThreadMgr* m_pThreadMgr;
    TfClientId    m_tfClientId;

    std::unique_ptr<CompositionManager>      m_pCompositionMgr;
    std::unique_ptr<DisplayAttributeProvider> m_pAttrProvider;
};
