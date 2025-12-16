#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

// Make sure at least one font is loaded....... I HATE THIS BUG
static std::wstring ResolveSvgFontFamily(const std::wstring& svgFont)
{
    // Default fallback
    if (svgFont.empty())
        return L"Arial";

    // Split by comma
    size_t start = 0;
    while (start < svgFont.size())
    {
        size_t end = svgFont.find(L',', start);
        if (end == std::wstring::npos)
            end = svgFont.size();

        std::wstring f = svgFont.substr(start, end - start);

        // Remove quotes
        f.erase(std::remove(f.begin(), f.end(), L'\"'), f.end());
        f.erase(std::remove(f.begin(), f.end(), L'\''), f.end());

        // Trim spaces
        while (!f.empty() && iswspace(f.front())) f.erase(f.begin());
        while (!f.empty() && iswspace(f.back()))  f.pop_back();

        // Map SVG generic families
        if (f == L"sans-serif") f = L"Arial";
        else if (f == L"serif") f = L"Times New Roman";
        else if (f == L"monospace") f = L"Consolas";

        if (!f.empty())
        {
            FontFamily fam(f.c_str());
            if (fam.IsAvailable())
                return f;
        }

        start = end + 1;
    }

    // Final fallback
    return L"Arial";
}

void GdiPlusRenderer::DrawText(const SvgText& text)
{
    GraphicsState state = graphics.Save();
    //ApplyTransform(graphics, text.transformAttribute);

    std::wstring family = ResolveSvgFontFamily(text.fontFamily);

    std::unique_ptr<FontFamily> fontFamily =
        std::make_unique<FontFamily>(family.c_str());

    if (!fontFamily->IsAvailable())
    {
        fontFamily = std::make_unique<FontFamily>(L"Arial");
    }

    Gdiplus::Font font(fontFamily.get(), text.fontSize,
        FontStyleRegular, UnitPixel);

    SolidBrush brush(text.fillColor);
    graphics.DrawString(text.text.c_str(), -1, &font,
        PointF(text.x, text.y), &brush);

    graphics.Restore(state);
}