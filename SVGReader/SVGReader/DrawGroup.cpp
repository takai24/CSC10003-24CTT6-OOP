#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawGroup(const SvgGroup &group)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, group.transformAttribute);
    for (const auto &child : group.children)
    {
        if (auto g = dynamic_cast<SvgGroup *>(child.get()))
        {
            g->Draw(*this);
            continue;
        }

        // Helper lambdas to compute effective color and alpha
        auto computeStrokeColor = [&](const Color &childStroke) -> Color
        {
            Color base = group.hasStroke ? group.strokeColor : childStroke;
            float baseA = base.GetAlpha() / 255.0f;
            float childA = childStroke.GetAlpha() / 255.0f;
            float groupA = group.hasStrokeOpacity ? group.strokeOpacity : 1.0f;
            float finalA = baseA * childA * groupA;
            int aInt = static_cast<int>(finalA * 255.0f + 0.5f);
            if (aInt < 0)
                aInt = 0;
            if (aInt > 255)
                aInt = 255;
            BYTE a = static_cast<BYTE>(aInt);
            return Color(a, base.GetR(), base.GetG(), base.GetB());
        };

        auto computeFillColor = [&](const Color &childFill) -> Color
        {
            Color base = group.hasFill ? group.fillColor : childFill;
            float baseA = base.GetAlpha() / 255.0f;
            float childA = childFill.GetAlpha() / 255.0f;
            float groupA = group.hasFillOpacity ? group.fillOpacity : 1.0f;
            float finalA = baseA * childA * groupA;
            int aInt = static_cast<int>(finalA * 255.0f + 0.5f);
            if (aInt < 0)
                aInt = 0;
            if (aInt > 255)
                aInt = 255;
            BYTE a = static_cast<BYTE>(aInt);
            return Color(a, base.GetR(), base.GetG(), base.GetB());
        };

        // Draw each shape using a temporary copy so we don't mutate original children
        if (auto line = dynamic_cast<SvgLine *>(child.get()))
        {
            SvgLine tmp = *line;
            tmp.strokeColor = computeStrokeColor(line->strokeColor);
            if (group.hasStrokeWidth)
                tmp.strokeWidth = line->strokeWidth * group.strokeWidth;
            DrawLine(tmp);
        }
        else if (auto rect = dynamic_cast<SvgRect *>(child.get()))
        {
            SvgRect tmp = *rect;
            tmp.strokeColor = computeStrokeColor(rect->strokeColor);
            tmp.fillColor = computeFillColor(rect->fillColor);
            if (group.hasStrokeWidth)
                tmp.strokeWidth = rect->strokeWidth * group.strokeWidth;
            DrawRect(tmp);
        }
        else if (auto circle = dynamic_cast<SvgCircle *>(child.get()))
        {
            SvgCircle tmp = *circle;
            tmp.strokeColor = computeStrokeColor(circle->strokeColor);
            tmp.fillColor = computeFillColor(circle->fillColor);
            if (group.hasStrokeWidth)
                tmp.strokeWidth = circle->strokeWidth * group.strokeWidth;
            DrawCircle(tmp);
        }
        else if (auto e = dynamic_cast<SvgEllipse *>(child.get()))
        {
            SvgEllipse tmp = *e;
            tmp.strokeColor = computeStrokeColor(e->strokeColor);
            tmp.fillColor = computeFillColor(e->fillColor);
            if (group.hasStrokeWidth)
                tmp.strokeWidth = e->strokeWidth * group.strokeWidth;
            DrawEllipse(tmp);
        }
        else if (auto pl = dynamic_cast<SvgPolyline *>(child.get()))
        {
            SvgPolyline tmp = *pl;
            tmp.strokeColor = computeStrokeColor(pl->strokeColor);
            tmp.fillColor = computeFillColor(pl->fillColor);
            if (group.hasStrokeWidth)
                tmp.strokeWidth = pl->strokeWidth * group.strokeWidth;
            DrawPolyline(tmp);
        }
        else if (auto pg = dynamic_cast<SvgPolygon *>(child.get()))
        {
            SvgPolygon tmp = *pg;
            tmp.strokeColor = computeStrokeColor(pg->strokeColor);
            tmp.fillColor = computeFillColor(pg->fillColor);
            if (group.hasStrokeWidth)
                tmp.strokeWidth = pg->strokeWidth * group.strokeWidth;
            DrawPolygon(tmp);
        }
        else if (auto path = dynamic_cast<SvgPath *>(child.get()))
        {
            SvgPath tmp = *path;
            tmp.strokeColor = computeStrokeColor(path->strokeColor);
            tmp.fillColor = computeFillColor(path->fillColor);
            if (group.hasStrokeWidth)
                tmp.strokeWidth = path->strokeWidth * group.strokeWidth;
            DrawPath(tmp);
        }
        else if (auto text = dynamic_cast<SvgText *>(child.get()))
        {
            SvgText tmp = *text;
            // For text we only consider fill color/opacities
            tmp.fillColor = computeFillColor(text->fillColor);
            DrawText(tmp);
        }
        else
        {
            // Fallback: call Draw on child which will route to the renderer
            child->Draw(*this);
        }
    }

    graphics.Restore(state);
}
