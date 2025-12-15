#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawEllipse(const SvgEllipse &e)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, e.transformAttribute);
    Pen pen(e.strokeColor, e.strokeWidth);
    SolidBrush brush(e.fillColor);
    graphics.FillEllipse(&brush, e.cx - e.rx, e.cy - e.ry, e.rx * 2.0f, e.ry * 2.0f);
    graphics.DrawEllipse(&pen, e.cx - e.rx, e.cy - e.ry, e.rx * 2.0f, e.ry * 2.0f);
    graphics.Restore(state);
}
