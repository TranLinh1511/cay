#include "GermanEngine.h"

namespace Cay {

// Dead key acute: ' + vowel -> accented char
// ' + e -> é, ' + E -> É
// ' + a -> á, ' + A -> Á
// ' + i -> í, ' + I -> Í
// ' + o -> ó, ' + O -> Ó
// ' + u -> ú, ' + U -> Ú
// ' + y -> ý, ' + Y -> Ý
// ' + ' -> ' (thoat, emit literal)
// ' + khac -> emit ' roi xu ly ky tu do binh thuong
static wchar_t GetAcute(wchar_t ch) {
    switch (ch) {
        case L'e': return L'\u00E9'; // é
        case L'E': return L'\u00C9'; // É
        case L'a': return L'\u00E1'; // á
        case L'A': return L'\u00C1'; // Á
        case L'i': return L'\u00ED'; // í
        case L'I': return L'\u00CD'; // Í
        case L'o': return L'\u00F3'; // ó
        case L'O': return L'\u00D3'; // Ó
        case L'u': return L'\u00FA'; // ú
        case L'U': return L'\u00DA'; // Ú
        case L'y': return L'\u00FD'; // ý
        case L'Y': return L'\u00DD'; // Ý
        default:   return 0;
    }
}

// Tra ve family key (lowercase) de group cac ky tu cung loai umlaut
// a/A -> 'a', u/U -> 'u', o/O -> 'o', s/S -> 's', khac -> 0
static wchar_t GetFamily(wchar_t ch) {
    switch (ch) {
        case L'a': case L'A': return L'a';
        case L'u': case L'U': return L'u';
        case L'o': case L'O': return L'o';
        case L's': case L'S': return L's';
        default: return 0;
    }
}

// Tra ve (special_char, trigger_count) cho tung ky tu
// Output umlaut dua theo ky tu dau cua cum (giu case):
//   aa/Aa/aA/AA -> ä/ä/ä/Ä, uu/Uu/uU/UU -> ü/ü/ü/Ü
// trigger=2: aa->ä, uu->ü, oo->ö  (hoac mix case trong cung family)
// trigger=3: sss->ß
static void GetRule(wchar_t ch, wchar_t& special, int& trig) {
    switch (ch) {
        case L'a': special = L'\u00E4'; trig = 2; return; // ä
        case L'A': special = L'\u00C4'; trig = 2; return; // Ä
        case L'u': special = L'\u00FC'; trig = 2; return; // ü
        case L'U': special = L'\u00DC'; trig = 2; return; // Ü
        case L'o': special = L'\u00F6'; trig = 2; return; // ö
        case L'O': special = L'\u00D6'; trig = 2; return; // Ö
        case L's': special = L'\u00DF'; trig = 3; return; // ß
        case L'S': special = L'\u00DF'; trig = 3; return; // ß
        default:   special = 0;         trig = 0; return;
    }
}

// Quy tac chung (ap dung cho ca umlaut va ß):
//   run < trig          -> giu nguyen (tung ky tu goc)
//   run chia het trig   -> run/trig ky tu dac biet (dua theo ky tu DAU cum)
//   run le (mod != 0)   -> bo 1, giu run-1 ky tu goc
//
// Cum duoc group theo family (a/A cung nhom, u/U cung nhom, ...),
// umlaut output dua theo ky tu dau tien cua cum.
// Vi du: Uu -> ü (U dau -> Ü? Khong, Uu la mixed -> lay ky tu dau = U -> Ü)
//        uU -> ü (u dau -> ü)
//        UU -> Ü, uu -> ü
void GermanEngine::BuildOutput(wchar_t* out, int& outLen) const {
    outLen = 0;
    int i = 0;
    while (i < _len) {
        wchar_t ch = _buf[i];
        wchar_t fam = GetFamily(ch);

        // Group theo family neu co, otherwise exact match
        int run = 1;
        if (fam != 0) {
            while (i + run < _len && GetFamily(_buf[i + run]) == fam) run++;
        } else {
            while (i + run < _len && _buf[i + run] == ch) run++;
        }

        wchar_t sp; int trig;
        GetRule(ch, sp, trig); // dua theo ky tu DAU cum de lay output umlaut

        if (!sp || run < trig) {
            for (int r = 0; r < run && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = _buf[i + r];
        } else if (run % trig == 0) {
            int count = run / trig;
            for (int r = 0; r < count && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = sp;
        } else if (trig == 3) {
            // Rieng ß: run khong chia het cho 3 -> lay het cac nhom du (count = run/trig)
            // thanh ß, phan du con lai (remainder) giu nguyen ky tu goc.
            // Vi du: run=4 -> "ß" + "s" = "ßs"; run=5 -> "ß" + "ss" = "ßss"
            int count = run / trig;
            int rem = run % trig;
            for (int r = 0; r < count && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = sp;
            for (int r = 0; r < rem && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = _buf[i + run - rem + r];
        } else {
            // Le: bo 1, giu run-1 ky tu goc
            for (int r = 0; r < run - 1 && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = _buf[i + r];
        }
        i += run;
    }
    out[outLen] = L'\0';
}

void GermanEngine::Inject(const wchar_t* newText, int newLen) {
    if (!OnInjectText) return;
    int common = 0;
    while (common < _lastOutputLen && common < newLen
           && _lastOutput[common] == newText[common])
        common++;
    int bs = _lastOutputLen - common;
    const wchar_t* suffix = newText + common;
    int suffixLen = newLen - common;
    if (bs == 0 && suffixLen == 0) return;
    OnInjectText(bs, suffix, suffixLen);
    _lastOutputLen = newLen;
    for (int k = 0; k < newLen; k++) _lastOutput[k] = newText[k];
    _lastOutput[newLen] = L'\0';
}

GermanEngine::GermanEngine() { ResetState(); }

void GermanEngine::ResetState() {
    _len = 0;
    _buf[0] = L'\0';
    _lastOutputLen = 0;
    _lastOutput[0] = L'\0';
    _pendingAcute = false;
}

void GermanEngine::ResetFull() { ResetState(); }

void GermanEngine::OnKeyDown(Cay::KeyEvent& e) {
    wchar_t ch = e.character;

    if (e.keyCode == Cay::KeyCode::Backspace) {
        // Neu dang cho vowel sau dead key ', huy dead key
        if (_pendingAcute) {
            _pendingAcute = false;
            e.handled = true;
            return;
        }
        if (_len > 0) {
            wchar_t last = _buf[_len - 1];
            wchar_t fam = GetFamily(last);
            wchar_t sp; int trig;
            GetRule(last, sp, trig);

            if (sp != 0 && fam != 0) {
                // Tinh do dai cum family o cuoi buffer
                int runEnd = 0;
                while (runEnd < _len && GetFamily(_buf[_len - 1 - runEnd]) == fam) runEnd++;

                // Xac dinh output hien tai cua cum:
                // - run % trig == 0: output la umlaut/ß (run/trig ky tu dac biet)
                // - run % trig != 0: output la run-1 ky tu goc (le)
                // - run < trig: output la run ky tu goc
                //
                // Backspace can xoa 1 ky tu OUTPUT:
                // TH1: output la umlaut (run%trig==0 && run>=trig) -> xoa 'trig' raw chars
                // TH2: output la ky tu goc -> xoa 1 raw char
                if (runEnd >= trig && runEnd % trig == 0) {
                    // Output cuoi la umlaut/ß -> xoa 'trig' raw chars de bo 1 umlaut
                    for (int d = 0; d < trig && _len > 0; d++) {
                        _len--;
                        _buf[_len] = L'\0';
                    }
                } else {
                    // Output van la ky tu goc -> xoa 1 raw char binh thuong
                    _len--;
                    _buf[_len] = L'\0';
                }
            } else {
                // Ky tu thuong -> xoa 1 ky tu binh thuong
                _len--;
                _buf[_len] = L'\0';
            }

            wchar_t outAfter[MAX_BUFFER]; int lenAfter = 0;
            BuildOutput(outAfter, lenAfter);
            Inject(outAfter, lenAfter);
            e.handled = true;
        }
        return;
    }

    if (e.keyCode == Cay::KeyCode::Space  || e.keyCode == Cay::KeyCode::Enter  ||
        e.keyCode == Cay::KeyCode::Escape || e.keyCode == Cay::KeyCode::Tab    ||
        e.keyCode == Cay::KeyCode::Left   || e.keyCode == Cay::KeyCode::Right  ||
        e.keyCode == Cay::KeyCode::Up     || e.keyCode == Cay::KeyCode::Down   ||
        e.keyCode == Cay::KeyCode::Home   || e.keyCode == Cay::KeyCode::End    ||
        e.keyCode == Cay::KeyCode::Delete) {
        _pendingAcute = false;
        ResetState();
        return;
    }

    // --- Xu ly dead key acute ---

    // Dang cho vowel sau '
    if (_pendingAcute) {
        _pendingAcute = false;

        if (ch == L'\'') {
            // '' -> emit literal '
            // Flush buffer truoc, reset, emit ' rieng
            wchar_t out[MAX_BUFFER]; int outLen = 0;
            BuildOutput(out, outLen);
            if (outLen > 0) Inject(out, outLen);
            ResetState();
            wchar_t lit[2] = { L'\'', L'\0' };
            OnInjectText(0, lit, 1);
            e.handled = true;
            return;
        }

        wchar_t accented = GetAcute(ch);
        if (accented != 0) {
            // Flush buffer truoc (German umlaut neu co), emit accented char rieng
            wchar_t out[MAX_BUFFER]; int outLen = 0;
            BuildOutput(out, outLen);
            if (outLen > 0) Inject(out, outLen);
            ResetState();
            wchar_t ac[2] = { accented, L'\0' };
            OnInjectText(0, ac, 1);
            e.handled = true;
            return;
        }

        // ' + ky tu khong hop le -> emit literal ' truoc, roi xu ly ky tu nhu binh thuong
        wchar_t out[MAX_BUFFER]; int outLen = 0;
        BuildOutput(out, outLen);
        if (outLen > 0) Inject(out, outLen);
        ResetState();
        wchar_t lit[2] = { L'\'', L'\0' };
        OnInjectText(0, lit, 1);
        // Fall through: tiep tuc xu ly ch binh thuong bên dưới
    }

    // Bat dau dead key khi gap dau '
    if (ch == L'\'') {
        // Flush buffer German hien tai (neu co), giu trang thai cho vowel
        wchar_t out[MAX_BUFFER]; int outLen = 0;
        BuildOutput(out, outLen);
        if (outLen > 0) Inject(out, outLen);
        ResetState();
        _pendingAcute = true;
        e.handled = true;
        return;
    }

    // --- Xu ly binh thuong (umlaut / ß) ---
    bool isAlpha = (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
    if (!isAlpha || ch == 0) { ResetState(); return; }

    if (_len < MAX_BUFFER - 1) {
        _buf[_len++] = ch;
        _buf[_len] = L'\0';
    }

    wchar_t out[MAX_BUFFER]; int outLen = 0;
    BuildOutput(out, outLen);
    Inject(out, outLen);
    e.handled = true;
}

} // namespace Cay
