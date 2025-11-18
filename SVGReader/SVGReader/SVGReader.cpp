#include "stdafx.h"

// Menu item IDs
#define ID_FILE_OPEN  9001
#define ID_FILE_EXIT  9002
#define ID_GROUP      9003
#define ID_PROJECT    9004

// Global SVGRenderer Instance
SvgRenderer* globalRenderer = nullptr;
Image* startupImage = nullptr;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
VOID OnPaint(HDC hdc);
bool OpenSVGFileDialog(wchar_t* outPath);

VOID OnPaint(HDC hdc)
{
    Graphics graphics(hdc);

    FontFamily fontFamily(L"Arial");
    Gdiplus::Font font(&fontFamily, 18, FontStyleRegular, UnitPixel);
    SolidBrush brush(Color(255, 0, 0, 0));
    graphics.DrawString(
        L"Chào mừng đến với SVG Reader (v1.0) của Nhóm 13. \nBắt đầu bằng cách ấn File -> Mở File...",
        -1,
        &font,
        PointF(20, 350),
        &brush
    );

    if (startupImage && startupImage->GetLastStatus() == Ok)
    {
        graphics.DrawImage(startupImage, 20, 20,
            startupImage->GetWidth(),
            startupImage->GetHeight());
    }

    // Actually draw the SVG
    if (globalRenderer)
    {
        globalRenderer->Draw(graphics);
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

    if (!hWnd) return 0;

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

// Actually creates Window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (message)
    {
    case WM_CREATE:
        startupImage = new Image(L"fit.png");
        return 0;
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
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        OnPaint(hdc);
        EndPaint(hWnd, &ps);
        return 0;

    case WM_DESTROY:
        if (startupImage) delete startupImage;
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
