#include "GermanEngine.h"

namespace Cay {

// Tra ve (special_char, trigger_count) cho tung ky tu
// trigger=2: aa->ä, uu->ü, oo->ö
// trigger=3: sss->ß
static void GetRule(wchar_t ch, wchar_t& special, int& trig) {
    switch (ch) {
        case L'a': special = L'\u00E4'; trig = 2; return;
        case L'A': special = L'\u00C4'; trig = 2; return;
        case L'u': special = L'\u00FC'; trig = 2; return;
        case L'U': special = L'\u00DC'; trig = 2; return;
        case L'o': special = L'\u00F6'; trig = 2; return;
        case L'O': special = L'\u00D6'; trig = 2; return;
        case L's': special = L'\u00DF'; trig = 3; return;
        case L'S': special = L'\u00DF'; trig = 3; return;
        default:   special = 0;         trig = 0; return;
    }
}

// Quy tac chung (ap dung cho ca umlaut va ß):
//   run < trig          -> giu nguyen
//   run chia het trig   -> run/trig ky tu dac biet
//   run le (mod != 0)   -> bo 1, giu run-1 ky tu goc
//
// Vi du umlaut (trig=2): a->a, aa->ä, aaa->aa, aaaa->ää
// Vi du ß     (trig=3): s->s, ss->ss, sss->ß, ssss->sss, ssssss->ßß
void GermanEngine::BuildOutput(wchar_t* out, int& outLen) const {
    outLen = 0;
    int i = 0;
    while (i < _len) {
        wchar_t ch = _buf[i];
        int run = 1;
        while (i + run < _len && _buf[i + run] == ch) run++;
        i += run;

        wchar_t sp; int trig;
        GetRule(ch, sp, trig);

        if (!sp || run < trig) {
            for (int r = 0; r < run && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = ch;
        } else if (run % trig == 0) {
            int count = run / trig;
            for (int r = 0; r < count && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = sp;
        } else {
            // Le: bo 1, giu run-1 ky tu goc
            for (int r = 0; r < run - 1 && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = ch;
        }
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
}

void GermanEngine::ResetFull() { ResetState(); }

void GermanEngine::OnKeyDown(Cay::KeyEvent& e) {
    wchar_t ch = e.character;

    if (e.keyCode == Cay::KeyCode::Backspace) {
        if (_len > 0) {
            // Tinh output hien tai truoc khi xoa
            wchar_t outBefore[MAX_BUFFER]; int lenBefore = 0;
            BuildOutput(outBefore, lenBefore);

            // Xoa 1 ky tu khoi buffer raw
            _len--;
            _buf[_len] = L'\0';

            // Tinh output sau khi xoa
            wchar_t outAfter[MAX_BUFFER]; int lenAfter = 0;
            BuildOutput(outAfter, lenAfter);

            // Truong hop 1: output khong doi sau khi xoa 1 raw
            // (vi du: ää -> xoa 1 'a' -> output van la ä)
            // -> xoa het cum raw con lai cung loai
            bool sameOutput = (lenBefore == lenAfter);
            for (int k = 0; k < lenBefore && sameOutput; k++)
                if (outBefore[k] != outAfter[k]) { sameOutput = false; }

            if (sameOutput && _len > 0) {
                wchar_t last = _buf[_len - 1];
                while (_len > 0 && _buf[_len - 1] == last) {
                    _len--;
                    _buf[_len] = L'\0';
                }
                BuildOutput(outAfter, lenAfter);
            }
            // Truong hop 2: output thay doi nhung ky tu cuoi dang la raw char "lo"
            // (vi du: oo -> xoa 1 -> con 'o', output = 'o' thay vi 'ö')
            // -> xoa luon ca cum raw do, khong de lai raw char lua
            else if (_len > 0 && lenAfter > 0) {
                wchar_t last = _buf[_len - 1];
                wchar_t sp; int trig;
                GetRule(last, sp, trig);

                if (sp != 0 && outAfter[lenAfter - 1] == last) {
                    // Dem so raw char 'last' lien tiep o cuoi buffer
                    int runLen = 0;
                    for (int k = _len - 1; k >= 0 && _buf[k] == last; k--) runLen++;

                    // Neu so raw < trig: dang "le", xoa het cum nay
                    if (runLen > 0 && runLen % trig != 0) {
                        while (_len > 0 && _buf[_len - 1] == last) {
                            _len--;
                            _buf[_len] = L'\0';
                        }
                        BuildOutput(outAfter, lenAfter);
                    }
                }
            }

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
        ResetState();
        return;
    }

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
