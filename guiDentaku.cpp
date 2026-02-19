#include <windows.h>
#include <stdio.h>
#include <cwctype>  // iswdigit 用

//四則演算を行うクラス
class Calculator{
    public : 
        int keisan(int x, char op, int y);
};
//四則演算を行うクラスのメンバ関数
int Calculator::keisan(int x, char op, int y){
    switch(op){
        case '+': return x + y;
        case '-': return x - y;
        case '*': return x * y;
        case '/': 
            if (y == 0){
                return x;
            }
            else return x / y;
        default: 
            return x;
    }
}

// 2. メッセージを処理する関数（窓口）
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    //電卓の現在の値を保持
    static int count = 0;

    switch (uMsg) {
        case WM_CREATE:{ // ウィンドウが作られた瞬間
            // ボタンを作成
            // 1. ボタンのラベル定義
            const wchar_t* labels[] = {
                L"7", L"8", L"9", L"/",
                L"4", L"5", L"6", L"*",
                L"1", L"2", L"3", L"-",
                L"0", L"C", L"=", L"+"
            };
            //証明情報を宣言
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            //ディスプレイの作成
            CreateWindowW(
                L"EDIT",   //クラス名らしい、既に用意されている箱
                L"0",       //表示される文字
                WS_VISIBLE | WS_CHILD | WS_BORDER | ES_RIGHT | ES_READONLY,  //スタイル
                10, 10, 230, 30, // 位置(x,y)とサイズ(幅,高さ)
                hwnd,
                (HMENU)200,
                hInst,
                NULL
            );
            // 3. ボタンを 4x4 でループ作成
            for (int i = 0; i < 16; i++) {
                int x = (i % 4) * 60 + 10;
                int y = (i / 4) * 60 + 50; // ディスプレイの下から配置するため +50

                CreateWindowW(
                    L"BUTTON", labels[i],
                    WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    x, y, 50, 50,
                    hwnd,
                    (HMENU)(UINT_PTR)(101 + i),
                    hInst,
                    NULL
                );
            }
        }break;

        case WM_COMMAND: {
            static int storedNum = 0;      // 1つ目の数字を保存
            static wchar_t currentOp = L'\0'; // 押された演算子 (+, -, *, /) を保存
            static bool isNewInput = true;    // 次に入力する数字が「書き始め」かどうか
            static Calculator calc;           // あなたが作ったCLIの計算クラス

            int wmId = LOWORD(wParam);
    
            if (wmId >= 101 && wmId <= 116) {   // ボタン（101～116）が押された場合
                const wchar_t* labels[] = {     // 表示用のラベル配列（WM_CREATEのものと同じ）
                    L"7", L"8", L"9", L"/",
                    L"4", L"5", L"6", L"*",
                    L"1", L"2", L"3", L"-",
                    L"0", L"C", L"=", L"+"
                };
                const wchar_t* selectedLabel = labels[wmId - 101];  // 1. どのボタンの文字を取得するか計算
                HWND hEdit = GetDlgItem(hwnd, 200);                 // 2. ディスプレイ（ID: 200）の「ハンドル」を取得

                if (wcscmp(selectedLabel, L"C") == 0) {         // 【Cボタン】 画面を "0" に戻す
                    SetWindowTextW(hEdit, L"0");
                } 
                else if (wcscmp(selectedLabel, L"=") == 0) {    // 【＝ボタン】 まだ何もしない（次のステップ）
                    wchar_t currentText[64];
                    GetWindowTextW(hEdit, currentText, 64);
                    int displayNum = _wtoi(currentText);
                    int result = calc.keisan(storedNum, (char)currentOp, displayNum);
                    swprintf(currentText, 64, L"%d", result);
                    SetWindowTextW(hEdit, currentText);
                    isNewInput = true;
                }
                else if (wcschr(L"+-*/", selectedLabel[0])) {   //　【演算子ボタン】
                    wchar_t currentText[64];
                    GetWindowTextW(hEdit, currentText, 64);     //　液晶の数字を取り出す
                    storedNum = _wtoi(currentText);             // 1. 液晶の数値を「最初の数」として記憶
                    currentOp = selectedLabel[0];               // 2. どの演算子か記憶
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