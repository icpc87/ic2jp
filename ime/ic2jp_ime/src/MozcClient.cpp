#include "MozcClient.h"

// ─── MozcClientStub ──────────────────────────────────────────────────────────
// Passthrough: returns the hiragana as its own single candidate.
// Replace each method body with real mozc::client::Client calls.
//
// Real implementation sketch:
//   #include "base/init_mozc.h"
//   #include "client/client.h"
//   #include "protocol/commands.pb.h"
//
//   mozc::client::Client m_mozcClient;
//
//   IMozcConverter::Result MozcClientReal::Convert(const std::wstring& hira) {
//       mozc::commands::KeyEvent ke;
//       std::string utf8;
//       mozc::Util::WideToUtf8(hira, &utf8);
//       ke.set_key_string(utf8);
//       mozc::commands::Output out;
//       m_mozcClient.SendKey(ke, &out);
//       return BuildResult(out);
//   }

IMozcConverter::Result MozcClientStub::Convert(const std::wstring& hiragana)
{
    m_hiragana = hiragana;
    m_last = Result{};
    m_last.preedit = hiragana;
    if (!hiragana.empty())
        m_last.candidates.push_back({ hiragana, L"" });
    m_last.focused = 0;
    return m_last;
}

IMozcConverter::Result MozcClientStub::SelectNext()
{
    // Stub has only one candidate; no-op.
    return m_last;
}

IMozcConverter::Result MozcClientStub::SelectPrev()
{
    return m_last;
}

std::wstring MozcClientStub::Commit(int /*index*/)
{
    std::wstring committed = m_hiragana;
    Reset();
    return committed;
}

void MozcClientStub::Reset()
{
    m_last     = Result{};
    m_hiragana.clear();
}
