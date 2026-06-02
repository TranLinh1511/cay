#pragma once
#include "CayTypes.h"

namespace Cay {

// ---------------------------------------------------------------------------
// GermanEngine
//
// Bo go tieng Duc don gian, khong dung CRT, khong cap phat dong.
//
// Quy tac:
//   aa -> ä  (aaa -> a + reset = "aa" literal -> tiep tuc -> "aaa" cho ra "aa")
//   uu -> ü
//   oo -> ö
//   ss -> ß
//
// Logic "n lan chu -> n-1 lan chu":
//   - Khi gap chu lap lan 2: thay the 2 chu bang dau bien (aa->ä)
//   - Khi gap chu lap lan 3: hoàn tác ve 2 chu goc (aaa->aa)
//   - Khi gap chu lap lan 4: lai thay the (aaaa->aä), v.v.
//   => tuong duong: so lan le -> lay dau bien, so lan chan -> lay ASCII goc
//
// ---------------------------------------------------------------------------
class GermanEngine {
public:
    GermanEngine();

    // Goi moi khi co phim duoc nhan xuong.
    void OnKeyDown(Cay::KeyEvent& e);

    // Reset toan bo state.
    void ResetFull();

    InjectTextFunc OnInjectText = nullptr;

private:
    // Buffer giu toan bo chuoi dang go (chua commit thanh tu)
    wchar_t _buf[MAX_BUFFER];
    int     _len;

    // Buffer output hien tai da inject len man hinh
    wchar_t _lastOutput[MAX_BUFFER];
    int     _lastOutputLen;

    // Thay the output cu bang output moi (tinh so backspace can thiet)
    void Inject(const wchar_t* newText, int newLen);

    // Reset state noi bo (khong gui phim nao)
    void ResetState();

    // Tra ve dau bien Duc cho cap ky tu, hoac 0 neu khong co
    static wchar_t GetUmlaut(wchar_t ch);

    // Rebuild output tu _buf (ap dung tat ca quy tac doi)
    void BuildOutput(wchar_t* out, int& outLen) const;
};

} // namespace Cay
