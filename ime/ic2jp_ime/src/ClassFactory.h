#pragma once
#include "Globals.h"

// IClassFactory implementation.  Creates ImeCore COM objects.
class ClassFactory : public IClassFactory
{
public:
    ClassFactory() : m_cRef(1) { InterlockedIncrement(&g_cDllRef); }

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) override;
    STDMETHOD_(ULONG, AddRef)()  override;
    STDMETHOD_(ULONG, Release)() override;

    // IClassFactory
    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppv) override;
    STDMETHOD(LockServer)(BOOL fLock) override;

private:
    ~ClassFactory() { InterlockedDecrement(&g_cDllRef); }
    ULONG m_cRef;
};
