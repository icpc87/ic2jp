#pragma once
#include <string>
#include <map>

// Roman-to-hiragana converter using longest-match lookup.
//
// Feed characters one at a time via Feed(); call Flush() on commit/cancel.
//
// Double-consonant rule: "kk" → "っ", buffer reset to "k"
// N rule: "nn" → "ん"; "n" before a vowel or y → keep buffering;
//          "n" before any other consonant → "ん"
class RomajiConverter
{
public:
    RomajiConverter();

    // Append ch to the internal buffer and attempt conversion.
    // Returns the hiragana produced so far (may be empty if still pending).
    // The internal buffer retains any unconverted tail.
    std::wstring Feed(wchar_t ch);

    // Forcibly convert whatever remains in the buffer (e.g. on Enter/commit).
    // Returns any hiragana produced; the buffer is cleared.
    std::wstring Flush();

    // Current unconverted buffer (read-only, for display).
    const std::wstring& Pending() const { return m_buf; }

    // Clear buffer without emitting text.
    void Reset() { m_buf.clear(); }

private:
    // Returns true if s is a prefix of at least one table key.
    bool HasPrefix(const std::wstring& s) const;

    std::map<std::wstring, std::wstring> m_table;
    std::wstring m_buf;
};
