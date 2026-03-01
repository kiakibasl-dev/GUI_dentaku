#include <windows.h>
#include <stdio.h>
#include <cwctype>  // iswdigit 用
#include <string>
#include <algorithm>

std::wstring baseExchange(int dec, int base){
    if(dec == 0) return L"0";

    std::wstring baseNnum;
    while(dec > 0){ 
        //余りを計算
        int rem = dec % base;
        wchar_t s;
        if(base > 10 && rem > 9) 
            s = L'A' + (rem - 10);
        else
            s = L'0' + rem;
        //10進数をbaseで割る
        dec /= base;

        baseNnum += s;
    }
    std::reverse(baseNnum.begin(), baseNnum.end());
    return baseNnum;
}


// 2. メッセージを処理する関数（窓口）
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    //電卓の現在の値を保持
    static int count = 0;

    switch (uMsg) {
        case WM_CREATE:{ // ウィンドウが作られた瞬間
            // ボタンを作成

            // --- メニューバーの作成 ---
            HMENU hMenu = CreateMenu();      // メニューバー本体
            HMENU hSubMenu = CreatePopupMenu(); // 「Mode」の中身

            // メニュー項目を追加 (ID: 301, 302, 303)
            AppendMenuW(hSubMenu, MF_STRING, 301, L"Calculator");
            AppendMenuW(hSubMenu, MF_STRING, 302, L"Base Converter");
            AppendMenuW(hSubMenu, MF_STRING, 303, L"Currency");

            // メニューバーに「Mode」という名前でサブメニューを登録
            AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, L"Mode");

            // ウィンドウにメニューバーをセット
            SetMenu(hwnd, hMenu);
            // --- メニューバーの作成 ---

            // 1. ボタンのラベル定義
            const wchar_t* labels[] = {
                L"7", L"8", L"9", L"BIN",
                L"4", L"5", L"6", L"OCT",
                L"1", L"2", L"3", L"HEX",
                L"0", L"C", L"=", L"DEC"
            };
            //証明情報を宣言
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
            HFONT hFont = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,    // 1. フォントを作成（大きさ 28、太字、MS ゴシックなど） 
                                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI  ");
            HWND hEdit = CreateWindowW(                             // --- 【2. ディスプレイの作成とフォント適用】 ---
                L"EDIT", L"0", 
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT | ES_READONLY,
                10, 10, 230, 40,
                hwnd, (HMENU)200, hInst, NULL
            );
            SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);    // ディスプレイにフォントをセット
            // WM_CREATE の中
            for (int i = 0; i < 16; i++) {
                int x = (i % 4) * 60 + 10;
                int y = (i / 4) * 60 + 60;

                DWORD style = WS_VISIBLE | WS_CHILD;
                if (wcschr(L"BINOCTHEXDEX=C", labels[i][0])) {               // 演算子（+, -, *, /, =, C）なら「自分で描く(OWNERDRAW)」スタイルにする
                    style |= BS_OWNERDRAW;
                } else {
                    style |= BS_PUSHBUTTON;
                }

                CreateWindowW(L"BUTTON", labels[i], style, x, y, 50, 50, hwnd, (HMENU)(UINT_PTR)(101 + i), hInst, NULL);
            }
        }break;

        case WM_DRAWITEM: {
            // lParam の中には「描き方の説明図(DRAWITEMSTRUCT)」へのポインタが入っている
            LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
            bool isPressed = (pDIS->itemState & ODS_SELECTED);// pDIS->itemState という変数の中に、現在の状態（選択中、押下中など）がビットで入っています。
            
            wchar_t buf[16];
            GetWindowTextW(pDIS->hwndItem, buf, 16);

            HBRUSH hBrush;
            if (wcscmp(buf, L"C") == 0) {
                // --- Cボタン（クリア）の場合：赤系 ---
                if (isPressed) {
                    hBrush = CreateSolidBrush(RGB(150, 0, 0)); // 濃い赤
                } else {
                    hBrush = CreateSolidBrush(RGB(255, 69, 0)); // 鮮やかな赤（オレンジレッド）
                }
            } else {
                // --- それ以外のボタン（演算子）：オレンジ系 ---
                if (isPressed) {
                    hBrush = CreateSolidBrush(RGB(200, 100, 0)); // 濃いオレンジ
                } else {
                    hBrush = CreateSolidBrush(RGB(255, 165, 0)); // 普通のオレンジ
                }
            }
            
            //塗る pDIS->hDC: 描画対象（キャンバス）&pDIS->rcItem: ボタンの大きさ（四角形）
            FillRect(pDIS->hDC, &pDIS->rcItem, hBrush);
            DeleteObject(hBrush);

            //枠線と文字を描く
            FrameRect(pDIS->hDC, &pDIS->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
            SetBkMode(pDIS->hDC, TRANSPARENT); // 文字の背景を透明に
            SetTextColor(pDIS->hDC, RGB(255, 255, 255)); // 文字を白に！
            DrawTextW(pDIS->hDC, buf, -1, &pDIS->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            break;
        }

        case WM_COMMAND: {
            static int storedNum = 0;      // 1つ目の数字を保存
            static wchar_t currentOp = L'\0'; // 押された演算子 (+, -, *, /) を保存
            static bool isNewInput = true;    // 次に入力する数字が「書き始め」かどうか

            int wmId = LOWORD(wParam);
    
            if (wmId >= 101 && wmId <= 116) {   // ボタン（101～116）が押された場合
                const wchar_t* labels[] = {     // 表示用のラベル配列（WM_CREATEのものと同じ）
                    L"7", L"8", L"9", L"BIN",
                    L"4", L"5", L"6", L"OCT",
                    L"1", L"2", L"3", L"HEX",
                    L"0", L"C", L"=", L"DEC"
                };
                const wchar_t* selectedLabel = labels[wmId - 101];  // 1. どのボタンの文字を取得するか計算
                HWND hEdit = GetDlgItem(hwnd, 200);                 // 2. ディスプレイ（ID: 200）の「ハンドル」を取得

                if (wcscmp(selectedLabel, L"C") == 0) {         // 【Cボタン】 計算過程をすべてリセット
                    SetWindowTextW(hEdit, L"0");
                                storedNum = 0;
                                currentOp = L'\0';
                                isNewInput = true;
                } 
                else if (wcscmp(selectedLabel, L"=") == 0) {    // 【＝ボタン】
                    //　ーーー　未定　ーーー
                }
                else if (wcscmp(selectedLabel, L"BIN") == 0) {   //　【BASEボタン】
                    wchar_t currentText[64];
                    GetWindowTextW(hEdit, currentText, 64);     //　液晶の数字を取り出す
                    std::wstring wresult = baseExchange(_wtoi(currentText), 2);
                    SetWindowTextW(hEdit,wresult.c_str());
                    isNewInput = true;                          // 次の数字は新しく書き始めてほしい
                }
                else if (wcscmp(selectedLabel, L"OCT") == 0) {   //　【BASEボタン】
                    wchar_t currentText[64];
                    GetWindowTextW(hEdit, currentText, 64);     //　液晶の数字を取り出す
                    std::wstring wresult = baseExchange(_wtoi(currentText), 8);
                    SetWindowTextW(hEdit,wresult.c_str());
                    isNewInput = true;                          // 3. 次の数字は新しく書き始めてほしいので、フラグを立てる
                }
                else if (wcscmp(selectedLabel, L"HEX") == 0) {   //　【BASEボタン】
                    wchar_t currentText[64];
                    GetWindowTextW(hEdit, currentText, 64);     //　液晶の数字を取り出す
                    std::wstring wresult = baseExchange(_wtoi(currentText), 16);
                    SetWindowTextW(hEdit,wresult.c_str());
                    isNewInput = true;                          // 3. 次の数字は新しく書き始めてほしいので、フラグを立てる
                }
                else if (iswdigit(selectedLabel[0])) {          // 【数字ボタン】 今の文字に付け足す
                    wchar_t currentText[64];

                    if(isNewInput){                             //画面をクリアして新しい数字を書く
                        SetWindowTextW(hEdit, selectedLabel);
                        isNewInput = false;
                    }else{
                        GetWindowTextW(hEdit, currentText, 64);
                        if (wcscmp(currentText, L"0") == 0) {       // 先頭が "0" なら、上書きする
                            SetWindowTextW(hEdit, selectedLabel);
                        } else {                                    // 先頭が "0" 以外なら、後ろにくっつける        
                            wcscat(currentText, selectedLabel);
                            SetWindowTextW(hEdit, currentText);
                        }
                    }
                }
            }
        }break;

        case WM_DESTROY: // 閉じるボタンが押された時
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// 1. プログラムの入り口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"MyWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW); // マウスカーソルの形を指定

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"calculator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        300, 400,   //幅と高さ
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    // 3. メッセージループ（ずっと回り続ける）
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}