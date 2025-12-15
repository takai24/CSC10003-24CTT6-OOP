#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawText(const SvgText &text)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, text.transformAttribute);
    FontFamily fontFamily(text.fontFamily.c_str());
    Gdiplus::Font font(&fontFamily, text.fontSize, FontStyleRegular, UnitPixel);
    SolidBrush brush(text.fillColor);
    graphics.DrawString(text.text.c_str(), -1, &font, PointF(text.x, text.y), &brush);
    graphics.Restore(state);
}
