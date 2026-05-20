#pragma once
#include <string>
#include <vector>

// Abstract Mozc conversion interface.
// Swap MozcClientStub for a real mozc::client::Client implementation once
// the Mozc libraries are linked.
class IMozcConverter
{
public:
    struct Candidate
    {
        std::wstring value;       // display text (kanji/kana)
        std::wstring annotation;  // reading or part-of-speech hint
    };

    struct Result
    {
        std::wstring              preedit;    // reading shown in the preedit
        std::vector<Candidate>    candidates; // ranked conversion candidates
        int                       focused{0}; // index of the top/selected candidate
    };

    virtual ~IMozcConverter() = default;

    // Send the full hiragana preedit string; returns updated candidates.
    // Called on every keystroke (live conversion).
    virtual Result Convert(const std::wstring& hiragana) = 0;

    // Cycle to the next candidate (Tab / arrow key).
    virtual Result SelectNext() = 0;

    // Cycle to the previous candidate.
    virtual Result SelectPrev() = 0;

    // Confirm candidate at index and return its committed text.
    virtual std::wstring Commit(int index = -1) = 0;

    // Discard all conversion state.
    virtual void Reset() = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Passthrough stub — returns the hiragana input unchanged as a single
// candidate.  Replace the body of each method with Mozc proto calls:
//
//   mozc::commands::KeyEvent ke;
//   ke.set_key_string(mozc::win32::ToUtf8(hiragana));
//   mozc::commands::Output out;
//   m_session->SendKey(ke, &out);
//   // populate Result from out.preedit() and out.candidates()
// ─────────────────────────────────────────────────────────────────────────────
class MozcClientStub : public IMozcConverter
{
public:
    Result Convert(const std::wstring& hiragana) override;
    Result SelectNext() override;
    Result SelectPrev() override;
    std::wstring Commit(int index = -1) override;
    void Reset() override;

private:
    Result       m_last;
    std::wstring m_hiragana;
};
