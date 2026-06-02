#include "GermanEngine.h"

namespace Cay {

wchar_t GermanEngine::GetUmlaut(wchar_t ch) {
    switch (ch) {
        case L'a': return L'\u00E4';
        case L'A': return L'\u00C4';
        case L'u': return L'\u00FC';
        case L'U': return L'\u00DC';
        case L'o': return L'\u00F6';
        case L'O': return L'\u00D6';
        case L's': return L'\u00DF';
        case L'S': return L'\u00DF';
        default:   return 0;
    }
}

void GermanEngine::BuildOutput(wchar_t* out, int& outLen) const {
    outLen = 0;
    int i = 0;
    while (i < _len) {
        wchar_t ch = _buf[i];
        wchar_t uml = GetUmlaut(ch);
        int run = 1;
        while (i + run < _len && _buf[i + run] == ch) run++;
        i += run;

        if (uml == 0) {
            for (int r = 0; r < run && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = ch;
        } else {
            int pairs = run / 2;
            int rem   = run % 2;
            for (int r = 0; r < pairs && outLen < MAX_BUFFER - 1; r++)
                out[outLen++] = uml;
            if (rem && outLen < MAX_BUFFER - 1)
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
    for (int i = 0; i < newLen; i++) _lastOutput[i] = newText[i];
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
            _len--;
            _buf[_len] = L'\0';
            wchar_t out[MAX_BUFFER];
            int outLen = 0;
            BuildOutput(out, outLen);
            Inject(out, outLen);
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

    wchar_t out[MAX_BUFFER];
    int outLen = 0;
    BuildOutput(out, outLen);
    Inject(out, outLen);
    e.handled = true;
}

} // namespace Cay
