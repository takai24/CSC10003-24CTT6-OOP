#include "stdafx.h"
#include "GdiPlusRenderer.h"
using namespace Gdiplus;

void GdiPlusRenderer::DrawPath(const SvgPath& path)
{
    if (!path.pathData) return;

    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, path.transformAttribute);

    RectF bounds;
    path.pathData->GetBounds(&bounds);

    if (path.fillColor.GetAlpha() > 0 || !path.fillAttributeString.empty())
    {
        Brush* brush = CreateBrush(path.fillAttributeString, path.fillColor, bounds);
        if (brush)
        {
            path.pathData->SetFillMode(path.fillMode);
            graphics.FillPath(brush, path.pathData.get());
            delete brush;
        }
    }

    if (path.strokeWidth > 0 && path.strokeColor.GetAlpha() > 0)
    {
        Pen pen(path.strokeColor, path.strokeWidth);
        pen.SetLineJoin(LineJoinRound);
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        graphics.DrawPath(&pen, path.pathData.get());
    }

    graphics.Restore(state);
}