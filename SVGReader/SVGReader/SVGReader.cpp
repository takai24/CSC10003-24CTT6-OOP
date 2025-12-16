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

// Button IDs
#define ID_BTN_ZOOM_IN 101
#define ID_BTN_ZOOM_OUT 102
#define ID_BTN_ROTATE 103
#define ID_BTN_RESET 104
#define ID_BTN_LEFT 105
#define ID_BTN_RIGHT 106
#define ID_BTN_UP 107
#define ID_BTN_DOWN 108

// Button's size
#define BTN_WIDTH 32
#define BTN_HEIGHT 32
#define BTN_ARROW_SIZE 32
#define BTN_GAP 10
#define BTN_Y_MARGIN 40

// Global SVGRenderer Instance
SvgRenderer* globalRenderer = nullptr;
Image* startupImage = nullptr;
// Default
float g_Scale = 1.0f;
float g_Angle = 0.0f;
float g_OffsetX = 0.0f;
float g_OffsetY = 0.0f;
// 3 7 8 10 12 16 17 
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID OnPaint(HDC hdc);
bool OpenSVGFileDialog(wchar_t* outPath);
void SetButtonsVisible(HWND hWnd, bool visible);

VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);
    graphics.Clear(Color(255, 255, 255, 255));
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);

    if (!globalRenderer)
    {
        FontFamily fontFamily(L"Arial");
        Gdiplus::Font font(&fontFamily, 18, FontStyleRegular, UnitPixel);
        SolidBrush brush(Color(255, 0, 0, 0));
        graphics.DrawString(
            L"Chào mừng đến với SVG Reader (v2.0) của Nhóm 13. \nBắt đầu bằng cách ấn File -> Mở File...",
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
    else
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

    // Window's Resolution
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    int windowWidth = screenWidth * 2 / 3;
    int windowHeight = screenHeight * 2 / 3;

    // Centered
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    // Create Window
    hWnd = CreateWindow(
        TEXT("SVGReaderWindow"),
        TEXT("SVG Reader"),
        WS_OVERLAPPEDWINDOW,
        windowX, windowY,
        windowWidth, windowHeight,
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
        ID_BTN_LEFT, ID_BTN_RIGHT, ID_BTN_UP, ID_BTN_DOWN };

    for (int id : btnIDs)
    {
        HWND hBtn = GetDlgItem(hWnd, id);
        if (hBtn)
            ShowWindow(hBtn, showCmd);
    }
}

// Actually creates Window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlType != ODT_BUTTON)
            break;

        HDC hdc = dis->hDC;
        RECT rc = dis->rcItem;

        COLORREF bgColor = RGB(200, 200, 200);
        if (dis->itemState & ODS_SELECTED)
            bgColor = RGB(150, 150, 150);

        const int RADIUS = 6;

        HBRUSH bgBrush = CreateSolidBrush(bgColor);
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));

        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);

        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, RADIUS, RADIUS);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);

        DeleteObject(bgBrush);
        DeleteObject(borderPen);

        if (dis->itemState & ODS_SELECTED)
            OffsetRect(&rc, 1, 1);

        const wchar_t* text = L"";
        HICON hIcon = NULL;

        switch (dis->CtlID)
        {
        case ID_BTN_ZOOM_IN:
            text = L"";
            hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_ZOOM_IN),
                IMAGE_ICON, BTN_HEIGHT, BTN_HEIGHT, LR_DEFAULTCOLOR);
            break;

        case ID_BTN_ZOOM_OUT:
            text = L"";
            hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_ZOOM_OUT),
                IMAGE_ICON, BTN_HEIGHT, BTN_HEIGHT, LR_DEFAULTCOLOR);
            break;

        case ID_BTN_ROTATE:
            text = L"";
            hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_ROTATE),
                IMAGE_ICON, BTN_HEIGHT, BTN_HEIGHT, LR_DEFAULTCOLOR);
            break;

        case ID_BTN_UP:
            text = L"";
            hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_ARROW_UP),
                IMAGE_ICON, BTN_HEIGHT, BTN_HEIGHT, LR_DEFAULTCOLOR);
            break;

        case ID_BTN_DOWN:
            text = L"";
            hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_ARROW_DOWN),
                IMAGE_ICON, BTN_HEIGHT, BTN_HEIGHT, LR_DEFAULTCOLOR);
            break;

        case ID_BTN_LEFT:
            text = L"";
            hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_ARROW_LEFT),
                IMAGE_ICON, BTN_HEIGHT, BTN_HEIGHT, LR_DEFAULTCOLOR);
            break;

        case ID_BTN_RIGHT:
            text = L"";
            hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_ARROW_RIGHT),
                IMAGE_ICON, BTN_HEIGHT, BTN_HEIGHT, LR_DEFAULTCOLOR);
            break;

        case ID_BTN_RESET:
            text = L"";
            hIcon = (HICON)LoadImage(GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDI_RESET),
                IMAGE_ICON, BTN_HEIGHT, BTN_HEIGHT, LR_DEFAULTCOLOR);
            break;
        }

        int padding = 0;
        int iconSize = BTN_HEIGHT - padding;

        if (hIcon)
        {
            int iconX = rc.left + (rc.right - rc.left - iconSize) / 2;
            int iconY = rc.top + (rc.bottom - rc.top - iconSize) / 2;

            if (dis->itemState & ODS_SELECTED)
            {
                iconX += 1;
                iconY += 1;
            }

            DrawIconEx(hdc, iconX, iconY, hIcon,
                iconSize, iconSize, 0, NULL, DI_NORMAL);

            DestroyIcon(hIcon);
        }

        RECT textRc = rc;
        textRc.left += iconSize + padding * 2;

        SetBkMode(hdc, TRANSPARENT);
        DrawText(hdc, text, -1, &textRc,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        if (hIcon)
            DestroyIcon(hIcon);

        return TRUE;
    }

    case WM_CREATE:
    {
        startupImage = new Image(L"fit.png");

        // Zoom / Rotate buttons
        CreateWindow(TEXT("BUTTON"), TEXT("Zoom +"),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, BTN_WIDTH, BTN_HEIGHT,
            hWnd, (HMENU)ID_BTN_ZOOM_IN, NULL, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Zoom -"),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, BTN_WIDTH, BTN_HEIGHT,
            hWnd, (HMENU)ID_BTN_ZOOM_OUT, NULL, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Rotate"),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, BTN_WIDTH, BTN_HEIGHT,
            hWnd, (HMENU)ID_BTN_ROTATE, NULL, NULL);

        // Arrow buttons
        CreateWindow(TEXT("BUTTON"), TEXT("Left"),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, BTN_ARROW_SIZE, BTN_ARROW_SIZE,
            hWnd, (HMENU)ID_BTN_LEFT, NULL, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Up"),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, BTN_ARROW_SIZE, BTN_ARROW_SIZE,
            hWnd, (HMENU)ID_BTN_UP, NULL, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Down"),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, BTN_ARROW_SIZE, BTN_ARROW_SIZE,
            hWnd, (HMENU)ID_BTN_DOWN, NULL, NULL);

        CreateWindow(TEXT("BUTTON"), TEXT("Right"),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, BTN_ARROW_SIZE, BTN_ARROW_SIZE,
            hWnd, (HMENU)ID_BTN_RIGHT, NULL, NULL);

        // Reset button
        CreateWindow(TEXT("BUTTON"), TEXT("Reset"),
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            0, 0, BTN_WIDTH, BTN_HEIGHT,
            hWnd, (HMENU)ID_BTN_RESET, NULL, NULL);

        // Hide buttons until SVG is loaded
        SetButtonsVisible(hWnd, false);

        return 0;
    }

    case WM_SIZE:
    {
        if (!GetDlgItem(hWnd, ID_BTN_ZOOM_IN))
            return 0;

        RECT rc;
        GetClientRect(hWnd, &rc);

        int clientW = rc.right - rc.left;
        int clientH = rc.bottom - rc.top;

        int y = clientH - BTN_HEIGHT - BTN_Y_MARGIN;
        if (y < 0)
            y = 0;

        int x = 10;

        MoveWindow(GetDlgItem(hWnd, ID_BTN_ZOOM_IN), x, y, BTN_WIDTH, BTN_HEIGHT, TRUE);
        x += BTN_WIDTH + BTN_GAP;

        MoveWindow(GetDlgItem(hWnd, ID_BTN_ZOOM_OUT), x, y, BTN_WIDTH, BTN_HEIGHT, TRUE);
        x += BTN_WIDTH + BTN_GAP;

        MoveWindow(GetDlgItem(hWnd, ID_BTN_LEFT), x, y, BTN_ARROW_SIZE, BTN_ARROW_SIZE, TRUE);
        MoveWindow(GetDlgItem(hWnd, ID_BTN_RIGHT), x + BTN_ARROW_SIZE * 2, y, BTN_ARROW_SIZE, BTN_ARROW_SIZE, TRUE);
        MoveWindow(GetDlgItem(hWnd, ID_BTN_UP), x + BTN_ARROW_SIZE, y - BTN_ARROW_SIZE, BTN_ARROW_SIZE, BTN_ARROW_SIZE, TRUE);
        MoveWindow(GetDlgItem(hWnd, ID_BTN_DOWN), x + BTN_ARROW_SIZE, y + BTN_ARROW_SIZE, BTN_ARROW_SIZE, BTN_ARROW_SIZE, TRUE);

        x += BTN_ARROW_SIZE * 3 + BTN_GAP;
        MoveWindow(GetDlgItem(hWnd, ID_BTN_ROTATE), x, y, BTN_WIDTH, BTN_HEIGHT, TRUE);
        x += BTN_WIDTH + BTN_GAP;
        MoveWindow(GetDlgItem(hWnd, ID_BTN_RESET), x, y, BTN_WIDTH, BTN_HEIGHT, TRUE);

        return 0;
    }

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

                // New drawer for new svg
                if (globalRenderer)
                {
                    delete globalRenderer;
                    globalRenderer = nullptr;
                }

                globalRenderer = new SvgRenderer();

                if (globalRenderer->Load(filePath))
                {
                    MessageBox(hWnd, L"File đã được mở.", L"Thành công!", MB_OK);

                    SetButtonsVisible(hWnd, true);
                    // Resize window to better fit the SVG content if document provides dimensions
                    const SvgDocument& doc = globalRenderer->GetDocument();
                    float svgW = doc.GetWidth();
                    float svgH = doc.GetHeight();
                    if (svgW > 0 && svgH > 0)
                    {
                        // desired client area (add margins for UI controls)
                        int marginW = 120;
                        int marginH = 140;
                        int desiredClientW = static_cast<int>(std::ceil(svgW)) + marginW;
                        int desiredClientH = static_cast<int>(std::ceil(svgH)) + marginH;

                        int screenW = GetSystemMetrics(SM_CXSCREEN);
                        int screenH = GetSystemMetrics(SM_CYSCREEN);
                        int maxW = screenW * 9 / 10;
                        int maxH = screenH * 9 / 10;
                        if (desiredClientW > maxW)
                            desiredClientW = maxW;
                        if (desiredClientH > maxH)
                            desiredClientH = maxH;

                        RECT wr = { 0, 0, desiredClientW, desiredClientH };
                        DWORD style = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
                        // window has menu bar
                        AdjustWindowRect(&wr, style, TRUE);
                        int winW = wr.right - wr.left;
                        int winH = wr.bottom - wr.top;
                        int winX = (screenW - winW) / 2;
                        int winY = (screenH - winH) / 2;
                        SetWindowPos(hWnd, NULL, winX, winY, winW, winH, SWP_NOZORDER | SWP_SHOWWINDOW);
                    }
                    else
                    {
                        SendMessage(hWnd, WM_SIZE, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
                    }
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
            MessageBox(hWnd, L"Gíao viên hướng dẫn: \n- Thầy Đỗ Nguyễn Kha \n- Thầy Mai Anh Tuấn \n- Thầy Phạm Nguyễn Sơn Tùng \n \nSVGReader phiên bản: 2.0", L"Thông tin Đồ án", MB_OK);
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