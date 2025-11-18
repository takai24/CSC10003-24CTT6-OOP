#include "stdafx.h"

// Menu item IDs
#define ID_FILE_OPEN  9001
#define ID_FILE_EXIT  9002
#define ID_GROUP      9003
#define ID_PROJECT    9004

// Global SVGRenderer instance
SvgRenderer* globalRenderer = nullptr;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID OnPaint(HDC hdc);
bool OpenSVGFileDialog(wchar_t* outPath);

VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);
    if (globalRenderer)
    {
        globalRenderer->Draw(graphics);
    }
}

// Open file dialog (only accepts .svg)
bool OpenSVGFileDialog(wchar_t* outPath)
{
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = outPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"SVG Files\0*.svg\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    return GetOpenFileName(&ofn) != 0;
}

// Main entry point
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
    HWND hWnd;
    MSG msg;
    WNDCLASS wndClass;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    // Init GDI+
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register window class
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = TEXT("SVGReaderWindow");

    RegisterClass(&wndClass);

    // Create menu bar
    HMENU hMenubar = CreateMenu();
    
    // Create File menu
    HMENU hFileMenu = CreateMenu();
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_OPEN, TEXT("Mở file"));
    AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, TEXT("Thoát"));
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hFileMenu, TEXT("File"));
    
    // Create About menu
    HMENU hAboutMenu = CreateMenu();
    AppendMenu(hAboutMenu, MF_STRING, ID_GROUP, TEXT("Nhóm"));
    AppendMenu(hAboutMenu, MF_STRING, ID_PROJECT, TEXT("Đồ Án"));
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hAboutMenu, TEXT("About"));

    // Create Window
    hWnd = CreateWindow(
        TEXT("SVGReaderWindow"),
        TEXT("SVG Reader"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        960, 540, // 1920x1080 x 1/2
        NULL,
        hMenubar,
        hInstance,
        NULL);

    if (!hWnd)
        return 0;

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    // Message loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Clean up
    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}

// Actually creates Window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_FILE_OPEN:
        {
            wchar_t filePath[MAX_PATH] = { 0 };
            if (OpenSVGFileDialog(filePath))
            {
                wchar_t* ext = wcsrchr(filePath, L'.');
                if (ext && _wcsicmp(ext, L".svg") != 0)
                {
                    MessageBox(hWnd, L"Chọn file .svg", L"Invalid File", MB_OK | MB_ICONWARNING);
                    return 0;
                }

                if (!globalRenderer)
                    globalRenderer = new SvgRenderer();
                
                if (globalRenderer->Load(filePath))
                {
                    MessageBox(hWnd, L"File đã được mở.", L"Thành công!", MB_OK | MB_ICONINFORMATION);
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                else
                {
                    MessageBox(hWnd, L"Failed to load SVG file", L"Error", MB_OK | MB_ICONERROR);
                }
            }
            return 0;
        }

        case ID_FILE_EXIT:
            PostQuitMessage(0);
            return 0;

        case ID_GROUP:
            MessageBox(hWnd, L"Nhóm số 13 \n\nCác thành viên: \nPhan Hữu Trọng Phúc - 24120122 \nĐỗ Chí Cao - 24120270 \nLâm Tuấn Khanh - 24120337 \nVõ Thành Hoan - 24120052", L"Thông tin Nhóm", MB_OK | MB_ICONINFORMATION);
            return 0;

        case ID_PROJECT:
            MessageBox(hWnd, L"GVHD: Thầy Đỗ Nguyễn Kha \nSVGReader phiên bản: 1.0", L"Thông tin Đồ án", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        OnPaint(hdc);
        EndPaint(hWnd, &ps);
        return 0;

    case WM_DESTROY:
        // Clean up
        if (globalRenderer)
        {
            delete globalRenderer;
            globalRenderer = nullptr;
        }
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
