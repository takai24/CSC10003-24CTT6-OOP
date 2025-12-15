#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawPath(const SvgPath &path)
{
    if (!path.pathData)
        return;

    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, path.transformAttribute);
    Pen pen(path.strokeColor, path.strokeWidth);
    SolidBrush brush(path.fillColor);
    graphics.FillPath(&brush, path.pathData.get());
    graphics.DrawPath(&pen, path.pathData.get());
    graphics.Restore(state);
}
