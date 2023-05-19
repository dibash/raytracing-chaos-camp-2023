#pragma once
#include <Windows.h>

#define WM_UPDATE_WINDOW_BUFFER (WM_USER + 0x0001)

class PixelBuffer {

    friend class Window;

    const int width;
    const int height;
    HWND hwnd = NULL;
    HDC hBackDC = NULL;
    HBITMAP hBackBitmap = NULL;
    DWORD* buff;

    void initBackBuffer() {
        HDC hWinDC = GetDC(hwnd);

        hBackDC = CreateCompatibleDC(hWinDC);
        hBackBitmap = CreateCompatibleBitmap(hWinDC, width, height);
        SetBitmapBits(hBackBitmap, width * height * 4, (const void*)(buff));

        SelectObject(hBackDC, hBackBitmap);
        ReleaseDC(hwnd, hWinDC);
    }

public:
    PixelBuffer(const PixelBuffer&) = delete;
    PixelBuffer& operator=(const PixelBuffer&) = delete;

    PixelBuffer(int w, int h) : width(w), height(h) {
        buff = new DWORD[size_t(width) * size_t(height)];
    }

    void init(HWND wnd) {
        hwnd = wnd;
        initBackBuffer();
    }

    ~PixelBuffer() {
        DeleteDC(hBackDC);
        DeleteObject(hBackBitmap);
        delete[] buff;
    }

    inline int w() const { return width; }
    inline int h() const { return height; }
    inline void setPixel(unsigned int x, unsigned int y, DWORD color) {
        buff[y * width + x] = color;
    }
};

class Window {
    HWND hwnd;
    int width;
    int height;
    PixelBuffer screenBuffer;

public:
    Window(int w, int h, const LPCSTR title, int posX = CW_USEDEFAULT, int posY = CW_USEDEFAULT)
        : width(w), height(h), screenBuffer(w, h)
    {
        // Register the window class.
        const LPCSTR CLASS_NAME = "Main Window Class";
        HINSTANCE hInstance = GetModuleHandle(NULL);

        WNDCLASS wc = { };

        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;

        RegisterClass(&wc);

        // Create the window.

        const long ex_style = 0;
        const long style = (WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX);

        hwnd = CreateWindowEx(
            ex_style,                     // Extended window style
            CLASS_NAME,                   // Window class
            title,                        // Window text
            style,                        // Window style
            posX, posY, width, height,    // Position and size
            NULL, NULL, hInstance, NULL   // Parent window, Menu, Instance handle, Additional application data
        );

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

        screenBuffer.init(hwnd);

        ShowWindow(hwnd, SW_SHOWDEFAULT);
    }

    void runOnce(size_t ms = 0)
    {
        MSG msg = {};
        if (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (ms) {
            Sleep(DWORD(ms));
        }
    }

    void run()
    {
        MSG msg = { };
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void setBufferPixel(unsigned int x, unsigned int y, DWORD color) {
        screenBuffer.setPixel(x, y, color);
    }

    void updateBuffer()
    {
        PostMessage(hwnd, WM_UPDATE_WINDOW_BUFFER, 0, 0);
    }

    int getWidth() const
    {
        return width;
    }

    int getHeight() const
    {
        return height;
    }

private:

    void showBuffer()
    {
        SetBitmapBits(screenBuffer.hBackBitmap, screenBuffer.width * screenBuffer.height * 4, (const void*)(screenBuffer.buff));
        BitBlt(GetDC(hwnd), 0, 0, screenBuffer.width, screenBuffer.height, screenBuffer.hBackDC, 0, 0, SRCCOPY);
    }

    void printScreenCoordinates(int x, int y)
    {
        printf("(%d, %d)\n", x, y);
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_PAINT:
            break;

        case WM_UPDATE_WINDOW_BUFFER:
            ((Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA))->showBuffer();
            break;

        case WM_LBUTTONDOWN:
            ((Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA))->printScreenCoordinates(LOWORD(lParam), HIWORD(lParam));
            break;
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};
