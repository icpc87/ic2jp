#include "RomajiConverter.h"

// ─── Conversion table ─────────────────────────────────────────────────────────
// Sorted longest-first so the greedy match picks "shi" before "si", etc.
// Keys are lowercase ASCII; values are Unicode hiragana.
static const wchar_t* const kTable[][2] = {
    // ── 3-char combinations first (longest) ──────────────────────────────────
    { L"sha",  L"しゃ" },  // しゃ
    { L"shi",  L"し"       },  // し
    { L"shu",  L"しゅ" },  // しゅ
    { L"she",  L"しぇ" },  // しぇ
    { L"sho",  L"しょ" },  // しょ
    { L"chi",  L"ち"       },  // ち
    { L"cha",  L"ちゃ" },  // ちゃ
    { L"chu",  L"ちゅ" },  // ちゅ
    { L"che",  L"ちぇ" },  // ちぇ
    { L"cho",  L"ちょ" },  // ちょ
    { L"tsu",  L"つ"       },  // つ
    { L"tchi", L"っち" },  // っち
    { L"ttsu", L"っつ" },  // っつ
    { L"kya",  L"きゃ" },  // きゃ
    { L"kyi",  L"きぃ" },  // きぃ
    { L"kyu",  L"きゅ" },  // きゅ
    { L"kye",  L"きぇ" },  // きぇ
    { L"kyo",  L"きょ" },  // きょ
    { L"gya",  L"ぎゃ" },  // ぎゃ
    { L"gyu",  L"ぎゅ" },  // ぎゅ
    { L"gyo",  L"ぎょ" },  // ぎょ
    { L"nya",  L"にゃ" },  // にゃ
    { L"nyu",  L"にゅ" },  // にゅ
    { L"nyo",  L"にょ" },  // にょ
    { L"hya",  L"ひゃ" },  // ひゃ
    { L"hyu",  L"ひゅ" },  // ひゅ
    { L"hyo",  L"ひょ" },  // ひょ
    { L"mya",  L"みゃ" },  // みゃ
    { L"myu",  L"みゅ" },  // みゅ
    { L"myo",  L"みょ" },  // みょ
    { L"rya",  L"りゃ" },  // りゃ
    { L"ryu",  L"りゅ" },  // りゅ
    { L"ryo",  L"りょ" },  // りょ
    { L"bya",  L"びゃ" },  // びゃ
    { L"byu",  L"びゅ" },  // びゅ
    { L"byo",  L"びょ" },  // びょ
    { L"pya",  L"ぴゃ" },  // ぴゃ
    { L"pyu",  L"ぴゅ" },  // ぴゅ
    { L"pyo",  L"ぴょ" },  // ぴょ
    { L"jya",  L"じゃ" },  // じゃ
    { L"jyu",  L"じゅ" },  // じゅ
    { L"jyo",  L"じょ" },  // じょ
    { L"dya",  L"ぢゃ" },  // ぢゃ
    { L"dyu",  L"ぢゅ" },  // ぢゅ
    { L"dyo",  L"ぢょ" },  // ぢょ
    // ── 2-char combinations ──────────────────────────────────────────────────
    { L"ka",   L"か" },  // か
    { L"ki",   L"き" },  // き
    { L"ku",   L"く" },  // く
    { L"ke",   L"け" },  // け
    { L"ko",   L"こ" },  // こ
    { L"ga",   L"が" },  // が
    { L"gi",   L"ぎ" },  // ぎ
    { L"gu",   L"ぐ" },  // ぐ
    { L"ge",   L"げ" },  // げ
    { L"go",   L"ご" },  // ご
    { L"sa",   L"さ" },  // さ
    { L"si",   L"し" },  // し (alternate)
    { L"su",   L"す" },  // す
    { L"se",   L"せ" },  // せ
    { L"so",   L"そ" },  // そ
    { L"za",   L"ざ" },  // ざ
    { L"zi",   L"じ" },  // じ
    { L"zu",   L"ず" },  // ず
    { L"ze",   L"ぜ" },  // ぜ
    { L"zo",   L"ぞ" },  // ぞ
    { L"ta",   L"た" },  // た
    { L"ti",   L"ち" },  // ち (alternate)
    { L"tu",   L"つ" },  // つ (alternate)
    { L"te",   L"て" },  // て
    { L"to",   L"と" },  // と
    { L"da",   L"だ" },  // だ
    { L"di",   L"ぢ" },  // ぢ
    { L"du",   L"づ" },  // づ
    { L"de",   L"で" },  // で
    { L"do",   L"ど" },  // ど
    { L"na",   L"な" },  // な
    { L"ni",   L"に" },  // に
    { L"nu",   L"ぬ" },  // ぬ
    { L"ne",   L"ね" },  // ね
    { L"no",   L"の" },  // の
    { L"ha",   L"は" },  // は
    { L"hi",   L"ひ" },  // ひ
    { L"hu",   L"ふ" },  // ふ (alternate)
    { L"fu",   L"ふ" },  // ふ
    { L"he",   L"へ" },  // へ
    { L"ho",   L"ほ" },  // ほ
    { L"ba",   L"ば" },  // ば
    { L"bi",   L"び" },  // び
    { L"bu",   L"ぶ" },  // ぶ
    { L"be",   L"べ" },  // べ
    { L"bo",   L"ぼ" },  // ぼ
    { L"pa",   L"ぱ" },  // ぱ
    { L"pi",   L"ぴ" },  // ぴ
    { L"pu",   L"ぷ" },  // ぷ
    { L"pe",   L"ぺ" },  // ぺ
    { L"po",   L"ぽ" },  // ぽ
    { L"ma",   L"ま" },  // ま
    { L"mi",   L"み" },  // み
    { L"mu",   L"む" },  // む
    { L"me",   L"め" },  // め
    { L"mo",   L"も" },  // も
    { L"ya",   L"や" },  // や
    { L"yu",   L"ゆ" },  // ゆ
    { L"yo",   L"よ" },  // よ
    { L"ra",   L"ら" },  // ら
    { L"ri",   L"り" },  // り
    { L"ru",   L"る" },  // る
    { L"re",   L"れ" },  // れ
    { L"ro",   L"ろ" },  // ろ
    { L"wa",   L"わ" },  // わ
    { L"wi",   L"ゐ" },  // ゐ
    { L"we",   L"ゑ" },  // ゑ
    { L"wo",   L"を" },  // を
    { L"ja",   L"じゃ" },  // じゃ (alternate)
    { L"ji",   L"じ" },        // じ
    { L"ju",   L"じゅ" },  // じゅ (alternate)
    { L"je",   L"じぇ" },  // じぇ
    { L"jo",   L"じょ" },  // じょ (alternate)
    { L"nn",   L"ん" },        // ん
    // ── 1-char vowels ─────────────────────────────────────────────────────────
    { L"a",    L"あ" },  // あ
    { L"i",    L"い" },  // い
    { L"u",    L"う" },  // う
    { L"e",    L"え" },  // え
    { L"o",    L"お" },  // お
    { L"n",    L"ん" },  // ん  (only when followed by non-vowel/y, see Feed)
};

// ─────────────────────────────────────────────────────────────────────────────

RomajiConverter::RomajiConverter()
{
    for (auto& row : kTable)
        m_table[row[0]] = row[1];
}

bool RomajiConverter::HasPrefix(const std::wstring& s) const
{
    // lower_bound gives the first key >= s; check if it starts with s.
    auto it = m_table.lower_bound(s);
    if (it != m_table.end() && it->first.substr(0, s.size()) == s)
        return true;
    return false;
}

std::wstring RomajiConverter::Feed(wchar_t ch)
{
    std::wstring result;

    // Normalise to lowercase for table lookup (uppercase is handled externally).
    wchar_t lc = (ch >= L'A' && ch <= L'Z') ? (ch + (L'a' - L'A')) : ch;
    m_buf += lc;

    for (;;)
    {
        // ── Exact match: emit and clear buffer ──────────────────────────────
        auto it = m_table.find(m_buf);
        if (it != m_table.end())
        {
            result += it->second;
            m_buf.clear();
            break;
        }

        // ── Double-consonant rule: "kk" → "っ", keep second 'k' ────────────
        // Applies to any consonant except 'n' (handled separately as "nn").
        if (m_buf.size() >= 2)
        {
            wchar_t c0 = m_buf[0];
            wchar_t c1 = m_buf[1];
            bool isConsonant = (c0 != L'a' && c0 != L'i' && c0 != L'u' &&
                                c0 != L'e' && c0 != L'o' && c0 != L'n');
            if (isConsonant && c0 == c1)
            {
                result  += L'っ';  // っ
                m_buf.erase(0, 1);     // drop the first consonant
                continue;              // re-try with shortened buffer
            }
        }

        // ── "n" rule: emit ん when next char is not a vowel or 'y' ──────────
        if (m_buf.size() >= 2 && m_buf[0] == L'n')
        {
            wchar_t next = m_buf[1];
            bool followedByVowelOrY = (next == L'a' || next == L'i' ||
                                       next == L'u' || next == L'e' ||
                                       next == L'o' || next == L'y');
            if (!followedByVowelOrY)
            {
                result += L'ん';  // ん
                m_buf.erase(0, 1);
                continue;
            }
        }

        // ── Partial prefix: wait for more input ─────────────────────────────
        if (HasPrefix(m_buf))
            break;

        // ── No match and no prefix: emit the first char as-is, retry ────────
        result += m_buf[0];
        m_buf.erase(0, 1);
        if (m_buf.empty()) break;
    }

    return result;
}

std::wstring RomajiConverter::Flush()
{
    std::wstring result;
    while (!m_buf.empty())
    {
        // Try exact match on remaining buffer
        auto it = m_table.find(m_buf);
        if (it != m_table.end())
        {
            result += it->second;
            m_buf.clear();
            break;
        }
        // Emit first character as-is
        result += m_buf[0];
        m_buf.erase(0, 1);
    }
    return result;
}
