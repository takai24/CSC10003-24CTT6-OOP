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

        // Fix common typos / variants
        if (f == L"Time New Romand") f = L"Times New Roman";
        if (f == L"Time New Roman") f = L"Times New Roman";
    
        if (!f.empty())
        {
            FontFamily fam(f.c_str());
            if (fam.IsAvailable())
                return f;
        }

        start = end + 1;
    }

    // Final fallback
    return L"Time New Roman";
}

void GdiPlusRenderer::DrawText(const SvgText& text)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, text.transformAttribute);

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

    // Create a string format to respect text anchor
    Gdiplus::StringFormat format;       
    if (text.textAnchor == "middle")
        format.SetAlignment(StringAlignmentCenter);
    else if (text.textAnchor == "end" || text.textAnchor == "right")
        format.SetAlignment(StringAlignmentFar);
    else
        format.SetAlignment(StringAlignmentNear);

    // Compute baseline adjustment: SVG y is baseline, GDI+ AddString origin is top
    REAL ascent = 0.0f;
    REAL emHeight = 1.0f;
    if (fontFamily && fontFamily->IsAvailable())
    {
        ascent = static_cast<REAL>(fontFamily->GetCellAscent(FontStyleRegular));
        emHeight = static_cast<REAL>(fontFamily->GetEmHeight(FontStyleRegular));
    }
    REAL ascentPx = (emHeight != 0.0f) ? (text.fontSize * ascent / emHeight) : 0.0f;

   
    Gdiplus::GraphicsPath path;
    path.AddString(text.text.c_str(), -1, fontFamily.get(), FontStyleRegular,
        text.fontSize, PointF(text.x, text.y - ascentPx), &format);

   
    if (text.fillColor.GetAlpha() > 0)
        graphics.FillPath(&brush, &path);

    if (text.strokeColor.GetAlpha() > 0 && text.strokeWidth > 0.0f)
    {
        Gdiplus::Pen pen(text.strokeColor, text.strokeWidth);
        pen.SetLineJoin(LineJoinRound);
        graphics.DrawPath(&pen, &path);
    }               
            
    graphics.Restore(state);
}