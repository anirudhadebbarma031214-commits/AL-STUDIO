#include <windows.h>
#include <mmsystem.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comdlg32.lib")

namespace {
constexpr int STEPS = 16;
constexpr int TRACKS = 4;
constexpr int STEP_ID = 1000;
constexpr int PLAY_ID = 2000;
constexpr int STOP_ID = 2001;
constexpr int EXPORT_ID = 2002;
constexpr int BPM_ID = 2003;
constexpr int RATE = 44100;
bool pattern[TRACKS][STEPS] = {};
HWND stepButtons[TRACKS][STEPS]{};
HWND bpmEdit = nullptr;
HWND mainWindow = nullptr;
std::vector<short> audio;
std::vector<unsigned char> wav;
const wchar_t* trackNames[TRACKS] = {L"KICK", L"SNARE", L"CLOSED HAT", L"BASS"};

int getBpm() {
    wchar_t text[32]{};
    GetWindowTextW(bpmEdit, text, 31);
    int bpm = _wtoi(text);
    return std::clamp(bpm, 60, 180);
}
void put16(std::vector<unsigned char>& out, uint16_t v) {
    out.push_back(static_cast<unsigned char>(v & 255));
    out.push_back(static_cast<unsigned char>((v >> 8) & 255));
}
void put32(std::vector<unsigned char>& out, uint32_t v) {
    put16(out, static_cast<uint16_t>(v & 65535));
    put16(out, static_cast<uint16_t>((v >> 16) & 65535));
}
void renderAudio() {
    const int bpm = getBpm();
    const int bars = 4;
    const int totalFrames = RATE * 60 * 4 * bars / bpm;
    audio.assign(static_cast<size_t>(totalFrames) * 2, 0);
    const double stepSeconds = 60.0 / bpm / 4.0;
    for (int track = 0; track < TRACKS; ++track) {
        for (int step = 0; step < STEPS * bars; ++step) {
            if (!pattern[track][step % STEPS]) continue;
            const int start = static_cast<int>(step * stepSeconds * RATE);
            const int length = std::min(static_cast<int>(0.22 * RATE), totalFrames - start);
            double phase = 0.0;
            for (int n = 0; n < length; ++n) {
                const double t = static_cast<double>(n) / RATE;
                double sample = 0.0;
                if (track == 0) {
                    const double freq = 42.0 + 100.0 * std::exp(-t * 22.0);
                    phase += 6.28318530718 * freq / RATE;
                    sample = std::sin(phase) * std::exp(-t * 13.0) * 0.92;
                } else if (track == 1) {
                    const double noise = (static_cast<double>((n * 1103515245u + step * 12345u) & 65535) / 32768.0) - 1.0;
                    sample = (noise * 0.72 + std::sin(6.28318530718 * 185.0 * t) * 0.28) * std::exp(-t * 19.0) * 0.65;
                } else if (track == 2) {
                    const double noise = (static_cast<double>((n * 1664525u + step * 1013904223u) & 65535) / 32768.0) - 1.0;
                    sample = noise * std::exp(-t * 42.0) * 0.26;
                } else {
                    sample = std::sin(6.28318530718 * 55.0 * t) * std::exp(-t * 7.0) * 0.42;
                }
                const int pos = (start + n) * 2;
                const int mixed = static_cast<int>(audio[pos]) + static_cast<int>(sample * 32767.0);
                const short value = static_cast<short>(std::clamp(mixed, -32768, 32767));
                audio[pos] = value;
                audio[pos + 1] = value;
            }
        }
    }
    wav.clear();
    wav.reserve(44 + audio.size() * sizeof(short));
    wav.insert(wav.end(), {'R','I','F','F'});
    put32(wav, static_cast<uint32_t>(36 + audio.size() * sizeof(short)));
    wav.insert(wav.end(), {'W','A','V','E','f','m','t',' '});
    put32(wav, 16); put16(wav, 1); put16(wav, 2);
    put32(wav, RATE); put32(wav, RATE * 4); put16(wav, 4); put16(wav, 16);
    wav.insert(wav.end(), {'d','a','t','a'});
    put32(wav, static_cast<uint32_t>(audio.size() * sizeof(short)));
    const auto* bytes = reinterpret_cast<const unsigned char*>(audio.data());
    wav.insert(wav.end(), bytes, bytes + audio.size() * sizeof(short));
}
void refreshSteps() {
    for (int r = 0; r < TRACKS; ++r)
        for (int c = 0; c < STEPS; ++c)
            if (stepButtons[r][c])
                SendMessageW(stepButtons[r][c], BM_SETCHECK, pattern[r][c] ? BST_CHECKED : BST_UNCHECKED, 0);
}
void makeButton(HWND parent, const wchar_t* text, int id, int x, int y, int w, int h, DWORD style = 0) {
    CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | style,
        x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
}
LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        mainWindow = hwnd;
        CreateWindowExW(0, L"STATIC", L"AL STUDIO", WS_CHILD | WS_VISIBLE,
            24, 16, 260, 38, hwnd, nullptr, nullptr, nullptr);
        CreateWindowExW(0, L"STATIC", L"BEAT LAB  /  WINDOWS MVP", WS_CHILD | WS_VISIBLE,
            26, 50, 280, 22, hwnd, nullptr, nullptr, nullptr);
        makeButton(hwnd, L"PLAY  >", PLAY_ID, 24, 88, 92, 34);
        makeButton(hwnd, L"STOP  []", STOP_ID, 124, 88, 92, 34);
        makeButton(hwnd, L"EXPORT WAV", EXPORT_ID, 224, 88, 122, 34);
        CreateWindowExW(0, L"STATIC", L"BPM", WS_CHILD | WS_VISIBLE,
            368, 95, 42, 22, hwnd, nullptr, nullptr, nullptr);
        bpmEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"120",
            WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER, 410, 90, 58, 28,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(BPM_ID)), GetModuleHandleW(nullptr), nullptr);
        for (int r = 0; r < TRACKS; ++r) {
            CreateWindowExW(0, L"STATIC", trackNames[r], WS_CHILD | WS_VISIBLE,
                24, 166 + r * 54, 112, 26, hwnd, nullptr, nullptr, nullptr);
            for (int c = 0; c < STEPS; ++c) {
                const int x = 136 + c * 39;
                const int y = 158 + r * 54;
                stepButtons[r][c] = CreateWindowExW(0, L"BUTTON", L"",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_PUSHLIKE,
                    x, y, 34, 38, hwnd,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(STEP_ID + r * STEPS + c)),
                    GetModuleHandleW(nullptr), nullptr);
            }
        }
        // A starter beat so the app opens with something to hear.
        for (int c : {0, 4, 8, 12}) pattern[0][c] = true;
        for (int c : {4, 12}) pattern[1][c] = true;
        for (int c : {2, 6, 10, 14}) pattern[2][c] = true;
        refreshSteps();
        return 0;
    }
    case WM_COMMAND: {
        const int id = LOWORD(wp);
        if (id >= STEP_ID && id < STEP_ID + TRACKS * STEPS) {
            const int index = id - STEP_ID;
            pattern[index / STEPS][index % STEPS] = (SendMessageW(reinterpret_cast<HWND>(lp), BM_GETCHECK, 0, 0) == BST_CHECKED);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (id == PLAY_ID) {
            renderAudio();
            PlaySoundW(reinterpret_cast<LPCWSTR>(wav.data()), nullptr, SND_MEMORY | SND_ASYNC | SND_LOOP);
            return 0;
        }
        if (id == STOP_ID) {
            PlaySoundW(nullptr, nullptr, 0);
            return 0;
        }
        if (id == EXPORT_ID) {
            renderAudio();
            OPENFILENAMEW ofn{};
            wchar_t fileName[MAX_PATH] = L"AL-STUDIO-beat.wav";
            ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = L"WAV Audio (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = fileName; ofn.nMaxFile = MAX_PATH;
            ofn.lpstrDefExt = L"wav"; ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            if (GetSaveFileNameW(&ofn)) {
                std::ofstream f(fileName, std::ios::binary);
                f.write(reinterpret_cast<const char*>(wav.data()), static_cast<std::streamsize>(wav.size()));
                if (f) MessageBoxW(hwnd, L"WAV exported successfully.", L"AL STUDIO", MB_OK | MB_ICONINFORMATION);
                else MessageBoxW(hwnd, L"Could not write the WAV file.", L"Export error", MB_OK | MB_ICONERROR);
            }
            return 0;
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        HBRUSH bg = CreateSolidBrush(RGB(10, 15, 29)); FillRect(dc, &rc, bg); DeleteObject(bg);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(30, 151, 255));
        HFONT titleFont = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HFONT old = static_cast<HFONT>(SelectObject(dc, titleFont));
        TextOutW(dc, 24, 14, L"AL STUDIO", 9);
        SelectObject(dc, old); DeleteObject(titleFont);
        SetTextColor(dc, RGB(154, 177, 207));
        TextOutW(dc, 26, 52, L"BEAT LAB / WINDOWS MVP", 22);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(35, 58, 88));
        HGDIOBJ oldPen = SelectObject(dc, pen);
        MoveToEx(dc, 24, 143, nullptr); LineTo(dc, 770, 143);
        SelectObject(dc, oldPen); DeleteObject(pen);
        EndPaint(hwnd, &ps); return 0;
    }
    case WM_DESTROY:
        PlaySoundW(nullptr, nullptr, 0);
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = windowProc; wc.hInstance = instance;
    wc.lpszClassName = L"ALStudioMainWindow";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(10, 15, 29));
    if (!RegisterClassW(&wc)) return 1;
    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"AL STUDIO | Beat Lab",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 450, nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 2;
    ShowWindow(hwnd, show); UpdateWindow(hwnd);
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
