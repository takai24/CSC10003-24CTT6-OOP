#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawLine(const SvgLine &line)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, line.transformAttribute);
    Pen pen(line.strokeColor, line.strokeWidth);
    graphics.DrawLine(&pen, line.x1, line.y1, line.x2, line.y2);
    graphics.Restore(state);
}
