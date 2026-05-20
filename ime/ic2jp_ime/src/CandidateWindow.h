#pragma once
#include "Globals.h"
#include <string>
#include <vector>

// Win32 popup window that displays conversion candidates.
//
// Lifecycle:
//   Create() once (registers window class + creates HWND)
//   Update()  on every conversion update
//   Show() / Hide() to make it appear/disappear
//   Destroy() on IME deactivation
//
// Positioning: caller supplies the caret POINT in screen coordinates
// (obtained via ITfContextView::GetTextExt inside an edit session).
class CandidateWindow
{
public:
    CandidateWindow();
    ~CandidateWindow();

    HRESULT Create(HINSTANCE hInst);
    void    Destroy();

    // Replace the displayed candidate list.  focusedIndex is 0-based.
    void Update(const std::vector<std::wstring>& candidates,
                int focusedIndex);

    // Show the window anchored below the caret.
    void Show(POINT caretScreenPt);

    // Hide without destroying.
    void Hide();

    bool IsVisible() const;

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg,
                                     WPARAM wp, LPARAM lp);
    void OnPaint();

    static constexpr wchar_t kClassName[] = L"IC2JP_CandidateWindow";
    static constexpr int kPadX  = 8;
    static constexpr int kPadY  = 4;
    static constexpr int kItemH = 22;

    HWND                     m_hwnd{nullptr};
    std::vector<std::wstring> m_candidates;
    int                      m_focused{0};
};
