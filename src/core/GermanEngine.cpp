#include "GermanEngine.h"

namespace Cay {

// ---------------------------------------------------------------------------
// GetUmlaut: tra ve ky tu Duc tuong ung khi gap doi
//   aa->ä  uu->ü  oo->ö  ss->ß
//   AA->Ä  UU->Ü  OO->Ö  SS->ß  (ss->ß du hoa hay thuong)
// ---------------------------------------------------------------------------
wchar_t GermanEngine::GetUmlaut(wchar_t ch) {
    switch (ch) {
        case L'a': return L'\u00E4'; // ä
        case L'A': return L'\u00C4'; // Ä
        case L'u': return L'\u00FC'; // ü
        case L'U': return L'\u00DC'; // Ü
        case L'o': return L'\u00F6'; // ö
        case L'O': return L'\u00D6'; // Ö
        case L's': return L'\u00DF'; // ß
        case L'S': return L'\u00DF'; // ß (SS cung ra ß)
        default:   return 0;
    }
}

// ---------------------------------------------------------------------------
// BuildOutput: duyet _buf tu trai sang phai, dem so lan lap lien tiep,
// ap dung quy tac:
//   run = 1 -> giu nguyen
//   run = 2 -> xuat 1 ky tu umlaut (neu co), hoac 2 ky tu goc (neu khong co umlaut)
//   run = 3 -> xuat 2 ky tu goc  (aaa -> aa)
//   run = 4 -> xuat 1 umlaut + 1 ky tu goc?  Khong: xuat umlaut + umlaut??
//              Theo yeu cau "aaa->aa", suy ra:
//              - chan/le: so cap = run/2, so du = run%2
//              - moi cap cho ra 1 umlaut (neu co)
//              - du 1 cho ra 1 ky tu goc
//   Nhu vay:
//     run=1 -> goc
//     run=2 -> umlaut
//     run=3 -> umlaut + goc
//     run=4 -> umlaut + umlaut
//     run=5 -> umlaut + umlaut + goc
//   ...
// ---------------------------------------------------------------------------
void GermanEngine::BuildOutput(wchar_t* out, int& outLen) const {
    outLen = 0;
    int i = 0;
    while (i < _len) {
        wchar_t ch = _buf[i];
        wchar_t uml = GetUmlaut(ch);
        // dem run
        int run = 1;
        while (i + run < _len && _buf[i + run] == ch) run++;
        i += run;

        if (uml == 0) {
            // Khong co umlaut, giu nguyen tat ca
            for (int r = 0; r < run && outLen < MAX_BUFFER - 1; r++) {
                out[outLen++] = ch;
            }
        } else {
            // Moi 2 chu -> 1 umlaut; neu du 1 chu -> 1 goc
            int pairs = run / 2;
            int rem   = run % 2;
            for (int r = 0; r < pairs && outLen < MAX_BUFFER - 1; r++) {
                out[outLen++] = uml;
            }
            if (rem && outLen < MAX_BUFFER - 1) {
                out[outLen++] = ch;
            }
        }
    }
    out[outLen] = L'\0';
}

// ---------------------------------------------------------------------------
// Inject: so sanh output moi vs cu, gui backspace can thiet + text moi
// ---------------------------------------------------------------------------
void GermanEngine::Inject(const wchar_t* newText, int newLen) {
    if (!OnInjectText) return;

    // Tim prefix chung de tinh so bs
    int common = 0;
    while (common < _lastOutputLen && common < newLen
           && _lastOutput[common] == newText[common]) {
        common++;
    }

    int bs = _lastOutputLen - common;
    const wchar_t* suffix = newText + common;
    int suffixLen = newLen - common;

    if (bs == 0 && suffixLen == 0) return; // Khong co gi thay doi

    OnInjectText(bs, suffix, suffixLen);

    // Cap nhat lastOutput
    _lastOutputLen = newLen;
    for (int i = 0; i < newLen; i++) _lastOutput[i] = newText[i];
    _lastOutput[newLen] = L'\0';
}

// ---------------------------------------------------------------------------
// Constructor / Reset
// ---------------------------------------------------------------------------
GermanEngine::GermanEngine() {
    ResetState();
}

void GermanEngine::ResetState() {
    _len = 0;
    _buf[0] = L'\0';
    _lastOutputLen = 0;
    _lastOutput[0] = L'\0';
}

void GermanEngine::ResetFull() {
    ResetState();
}

// ---------------------------------------------------------------------------
// OnKeyDown: xu ly phim
// ---------------------------------------------------------------------------
void GermanEngine::OnKeyDown(Cay::KeyEvent& e) {
    wchar_t ch = e.character;

    // Phim Backspace
    if (e.keyCode == Cay::KeyCode::Backspace) {
        if (_len > 0) {
            _len--;
            _buf[_len] = L'\0';

            wchar_t out[MAX_BUFFER];
            int outLen = 0;
            BuildOutput(out, outLen);
            Inject(out, outLen);
            e.handled = true;
        }
        // Neu buffer rong, de phim Backspace di qua binh thuong (khong handled)
        return;
    }

    // Phim commit (Space, Enter, Escape, Tab, mui ten, ...)
    // -> flush buffer, de phim di qua
    if (e.keyCode == Cay::KeyCode::Space   ||
        e.keyCode == Cay::KeyCode::Enter   ||
        e.keyCode == Cay::KeyCode::Escape  ||
        e.keyCode == Cay::KeyCode::Tab     ||
        e.keyCode == Cay::KeyCode::Left    ||
        e.keyCode == Cay::KeyCode::Right   ||
        e.keyCode == Cay::KeyCode::Up      ||
        e.keyCode == Cay::KeyCode::Down    ||
        e.keyCode == Cay::KeyCode::Home    ||
        e.keyCode == Cay::KeyCode::End     ||
        e.keyCode == Cay::KeyCode::Delete) {
        ResetState();
        return;
    }

    // Chi xu ly ky tu chu cai (a-z, A-Z)
    bool isAlpha = (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z');
    if (!isAlpha || ch == 0) {
        // Ky tu khac (so, ky hieu,...) -> commit va de qua
        ResetState();
        return;
    }

    // Them ky tu vao buffer
    if (_len < MAX_BUFFER - 1) {
        _buf[_len++] = ch;
        _buf[_len] = L'\0';
    }

    // Build output va inject
    wchar_t out[MAX_BUFFER];
    int outLen = 0;
    BuildOutput(out, outLen);
    Inject(out, outLen);

    e.handled = true;
}

} // namespace Cay
