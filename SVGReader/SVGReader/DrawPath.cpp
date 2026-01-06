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
    
    RectF bounds;
    path.pathData->GetBounds(&bounds);
    auto brush = CreateFillBrush(path.fillUrl, path.fillColor, path.fillOpacity, bounds);

    if (brush && (path.fillColor.GetAlpha() > 0 || !path.fillUrl.empty()))
    {
        graphics.FillPath(brush.get(), path.pathData.get());
    }

    if (path.strokeColor.GetAlpha() > 0 && path.strokeWidth > 0.0f)
    {
        pen.SetLineJoin(LineJoinRound);
        pen.SetStartCap(LineCapRound);
        pen.SetEndCap(LineCapRound);
        pen.SetAlignment(PenAlignmentInset);
        graphics.DrawPath(&pen, path.pathData.get());
    }
    graphics.Restore(state);
}
