#include "stdafx.h"
#include "SvgReader.h"
#include "SvgElementFactory.h"
#include "GdiPlusRenderer.h"
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <commdlg.h>

// Menu item IDs
#define ID_FILE_OPEN 9001
#define ID_FILE_EXIT 9002
#define ID_GROUP 9003
#define ID_PROJECT 9004

#define ID_BTN_ZOOM_IN   101
#define ID_BTN_ZOOM_OUT  102
#define ID_BTN_ROTATE    103
#define ID_BTN_RESET     104
#define ID_BTN_LEFT      105
#define ID_BTN_RIGHT     106
#define ID_BTN_UP        107
#define ID_BTN_DOWN      108

// Global SVGRenderer Instance
SvgRenderer* globalRenderer = nullptr;
Image* startupImage = nullptr;
// Default
float g_Scale = 1.0f;
float g_Angle = 0.0f;
float g_OffsetX = 0.0f;
float g_OffsetY = 0.0f;

// Check if is first run
bool isFirstRun = true;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID OnPaint(HDC hdc);
bool OpenSVGFileDialog(wchar_t* outPath);
void SetButtonsVisible(HWND hWnd, bool visible);

VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);
    graphics.Clear(Color(255, 255, 255, 255));

    if (isFirstRun)
    {
        FontFamily fontFamily(L"Arial");
        Gdiplus::Font font(&fontFamily, 18, FontStyleRegular, UnitPixel);
        SolidBrush brush(Color(255, 0, 0, 0));
        graphics.DrawString(
            L"Chào mừng đến với SVG Reader (v1.0) của Nhóm 13. \nBắt đầu bằng cách ấn File -> Mở File...",
            -1,
            &font,
            PointF(20, 350),
            &brush);

        if (startupImage && startupImage->GetLastStatus() == Ok)
        {
            graphics.DrawImage(startupImage, 20, 20,
                startupImage->GetWidth(),
                startupImage->GetHeight());
        }
    }

    // Actually draw the SVG via new OOP renderer
    if (globalRenderer)
    {
        GdiPlusRenderer renderer(graphics);
        // Scale 90%
        graphics.ScaleTransform(0.9f, 0.9f);
        // Move center 
        RECT rect;
        GetClientRect(WindowFromDC(hdc), &rect);
        float g_CenterX = (float)(rect.right - rect.left) / 2.0f;
        float g_CenterY = (float)(rect.bottom - rect.top) / 2.0f;

        graphics.TranslateTransform(g_CenterX, g_CenterY);
        // Pan
        graphics.TranslateTransform(g_OffsetX, g_OffsetY);
        // Rotate
        graphics.RotateTransform(g_Angle);
        // Zoom
        graphics.ScaleTransform(g_Scale, g_Scale);

        graphics.TranslateTransform(-g_CenterX, -g_CenterY);

        globalRenderer->GetDocument().Render(renderer);
        graphics.ResetTransform();
    }
}

// Open file dialog (accepts .svg only)
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

// WinMain Entry
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow)
{
    HWND hWnd;
    MSG msg;
    WNDCLASS wndClass;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    // Init GDI+
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register Window Class
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = TEXT("SVGReaderWindow");

    RegisterClass(&wndClass);

    // Create Menu bar
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

    // Shut down
    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}

void SetButtonsVisible(HWND hWnd, bool visible)
{
    int showCmd = visible ? SW_SHOW : SW_HIDE;
    int btnIDs[] = {
        ID_BTN_ZOOM_IN, ID_BTN_ZOOM_OUT, ID_BTN_ROTATE, ID_BTN_RESET,
        ID_BTN_LEFT, ID_BTN_RIGHT, ID_BTN_UP, ID_BTN_DOWN
    };

    for (int id : btnIDs) {
        HWND hBtn = GetDlgItem(hWnd, id);
        if (hBtn) ShowWindow(hBtn, showCmd);
    }
}

// Actually creates Window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_CREATE:
        startupImage = new Image(L"fit.png");

        CreateWindow(TEXT("BUTTON"), TEXT("Zoom +"), WS_CHILD, 100, 400, 80, 30, hWnd, (HMENU)ID_BTN_ZOOM_IN, NULL, NULL);
        CreateWindow(TEXT("BUTTON"), TEXT("Zoom -"), WS_CHILD, 200, 400, 80, 30, hWnd, (HMENU)ID_BTN_ZOOM_OUT, NULL, NULL);
        CreateWindow(TEXT("BUTTON"), TEXT("Rotate"), WS_CHILD, 300, 400, 80, 30, hWnd, (HMENU)ID_BTN_ROTATE, NULL, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("<"), WS_CHILD, 400, 400, 30, 30, hWnd, (HMENU)ID_BTN_LEFT, NULL, NULL);
        CreateWindow(TEXT("BUTTON"), TEXT("^"), WS_CHILD, 450, 375, 30, 30, hWnd, (HMENU)ID_BTN_UP, NULL, NULL);
        CreateWindow(TEXT("BUTTON"), TEXT("v"), WS_CHILD, 450, 425, 30, 30, hWnd, (HMENU)ID_BTN_DOWN, NULL, NULL);
        CreateWindow(TEXT("BUTTON"), TEXT(">"), WS_CHILD, 500, 400, 30, 30, hWnd, (HMENU)ID_BTN_RIGHT, NULL, NULL);
        CreateWindow(TEXT("BUTTON"), TEXT("Reset"), WS_CHILD, 550, 400, 80, 30, hWnd, (HMENU)ID_BTN_RESET, NULL, NULL);
        break;

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
                    MessageBox(hWnd, L"Chọn file .svg", L"Invalid File", MB_OK);
                    return 0;
                }

                if (!globalRenderer)
                    globalRenderer = new SvgRenderer();

                if (globalRenderer->Load(filePath))
                {
                    MessageBox(hWnd, L"File đã được mở.", L"Thành công!", MB_OK);

                    isFirstRun = false;
                    SetButtonsVisible(hWnd, true);
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                else
                {
                    MessageBox(hWnd, L"Failed to load SVG file", L"Error", MB_OK);
                }
            }
            return 0;
        }

        case ID_FILE_EXIT:
            PostQuitMessage(0);
            return 0;

        case ID_GROUP:
            MessageBox(hWnd, L"Nhóm số 13 \n\nCác thành viên: \nPhan Hữu Trọng Phúc - 24120122 \nĐỗ Chí Cao - 24120270 \nLâm Tuấn Khanh - 24120337 \nVõ Thành Hoan - 24120052", L"Thông tin Nhóm", MB_OK);
            return 0;

        case ID_PROJECT:
            MessageBox(hWnd, L"GVHD: Thầy Đỗ Nguyễn Kha \nSVGReader phiên bản: 1.0", L"Thông tin Đồ án", MB_OK);
            return 0;

        case ID_BTN_ZOOM_IN:
            g_Scale *= 1.1f;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_BTN_ZOOM_OUT:
            g_Scale /= 1.1f;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_BTN_ROTATE:
            g_Angle += 45.0f;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_BTN_LEFT:
            g_OffsetX -= 50.0f;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_BTN_RIGHT:
            g_OffsetX += 50.0f;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_BTN_UP:
            g_OffsetY -= 50.0f;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_BTN_DOWN:
            g_OffsetY += 50.0f;
            InvalidateRect(hWnd, NULL, TRUE);
            break;

        case ID_BTN_RESET:
            g_Scale = 1.0f;
            g_Angle = 0.0f;
            g_OffsetX = 0.0f;
            g_OffsetY = 0.0f;
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        OnPaint(hdc);
        EndPaint(hWnd, &ps);
        return 0;

    case WM_DESTROY:
        if (startupImage)
            delete startupImage;
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