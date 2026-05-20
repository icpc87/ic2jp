#include "CandidateWindow.h"
#include <algorithm>

CandidateWindow::CandidateWindow() = default;

CandidateWindow::~CandidateWindow()
{
    Destroy();
}

HRESULT CandidateWindow::Create(HINSTANCE hInst)
{
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = kClassName;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);

    // Ignore ERROR_CLASS_ALREADY_EXISTS — fine if multiple IME instances exist.
    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        kClassName, nullptr,
        WS_POPUP | WS_BORDER,
        0, 0, 200, kItemH,
        nullptr, nullptr, hInst, this);

    return m_hwnd ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

void CandidateWindow::Destroy()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void CandidateWindow::Update(const std::vector<std::wstring>& candidates,
                              int focusedIndex)
{
    m_candidates = candidates;
    m_focused    = focusedIndex;

    if (!m_hwnd) return;

    if (candidates.empty())
    {
        Hide();
        return;
    }

    // Compute required window width from longest candidate string.
    HDC hdc = GetDC(m_hwnd);
    int maxW = 100;
    for (auto& c : m_candidates)
    {
        SIZE sz = {};
        GetTextExtentPoint32W(hdc, c.c_str(), static_cast<int>(c.size()), &sz);
        maxW = std::max(maxW, sz.cx + kPadX * 2 + 24 /* index number */);
    }
    ReleaseDC(m_hwnd, hdc);

    int height = kItemH * static_cast<int>(m_candidates.size()) + kPadY * 2;
    SetWindowPos(m_hwnd, nullptr, 0, 0, maxW, height,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CandidateWindow::Show(POINT caretScreenPt)
{
    if (!m_hwnd) return;

    // Position the window just below the caret.
    RECT rc;
    GetWindowRect(m_hwnd, &rc);
    int w = rc.right  - rc.left;
    int h = rc.bottom - rc.top;

    // Keep within screen bounds.
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = std::min(caretScreenPt.x, screenW - w);
    int y = caretScreenPt.y + 2;  // 2px gap below caret
    if (y + h > screenH) y = caretScreenPt.y - h - 2;  // flip above if clipped

    SetWindowPos(m_hwnd, HWND_TOPMOST, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void CandidateWindow::Hide()
{
    if (m_hwnd)
        ShowWindow(m_hwnd, SW_HIDE);
}

bool CandidateWindow::IsVisible() const
{
    return m_hwnd && IsWindowVisible(m_hwnd);
}

// ─── WndProc ─────────────────────────────────────────────────────────────────

LRESULT CALLBACK CandidateWindow::WndProc(HWND hwnd, UINT msg,
                                           WPARAM wp, LPARAM lp)
{
    CandidateWindow* self = nullptr;

    if (msg == WM_CREATE)
    {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
        self = static_cast<CandidateWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(self));
    }
    else
    {
        self = reinterpret_cast<CandidateWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    switch (msg)
    {
    case WM_PAINT:
        if (self) self->OnPaint();
        return 0;

    case WM_ERASEBKGND:
        return 1;  // handled in OnPaint
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void CandidateWindow::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT clientRc;
    GetClientRect(m_hwnd, &clientRc);

    // Background
    HBRUSH hBrBg = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(hdc, &clientRc, hBrBg);
    DeleteObject(hBrBg);

    // Each candidate row
    HFONT hFont = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HFONT hOld  = static_cast<HFONT>(SelectObject(hdc, hFont));

    int y = kPadY;
    for (int i = 0; i < static_cast<int>(m_candidates.size()); ++i)
    {
        RECT rowRc = { clientRc.left, y, clientRc.right, y + kItemH };
        bool focused = (i == m_focused);

        // Highlight focused row
        if (focused)
        {
            HBRUSH hBrHl = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
            FillRect(hdc, &rowRc, hBrHl);
            DeleteObject(hBrHl);
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
        else
        {
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        }

        // Index number (1-based)
        wchar_t idx[4];
        swprintf_s(idx, L"%d.", i + 1);
        RECT idxRc = { rowRc.left + kPadX, rowRc.top,
                       rowRc.left + kPadX + 20, rowRc.bottom };
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, idx, -1, &idxRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Candidate text
        RECT textRc = { idxRc.right, rowRc.top,
                        rowRc.right - kPadX, rowRc.bottom };
        DrawTextW(hdc, m_candidates[i].c_str(), -1, &textRc,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        y += kItemH;
    }

    SelectObject(hdc, hOld);
    EndPaint(m_hwnd, &ps);
}
