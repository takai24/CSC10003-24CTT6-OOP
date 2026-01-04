#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawGroup(const SvgGroup &group)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, group.transformAttribute);
    auto computeStrokeColor = [&](const Color &childStroke) -> Color
    {
        Color base = group.hasStroke ? group.strokeColor : childStroke;
        float baseA = base.GetAlpha() / 255.0f;
        float childA = childStroke.GetAlpha() / 255.0f;
        if (group.hasStroke && childStroke.GetAlpha() == 0)
            childA = 1.0f;
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

    for (const auto &child : group.children)
    {
        if (auto g = dynamic_cast<SvgGroup *>(child.get()))
        {
            bool old_hasStroke = g->hasStroke;
            Color old_strokeColor = g->strokeColor;
            bool old_hasFill = g->hasFill;
            Color old_fillColor = g->fillColor;
            bool old_hasStrokeWidth = g->hasStrokeWidth;
            float old_strokeWidth = g->strokeWidth;
            bool old_hasStrokeOpacity = g->hasStrokeOpacity;
            float old_strokeOpacity = g->strokeOpacity;
            bool old_hasFillOpacity = g->hasFillOpacity;
            float old_fillOpacity = g->fillOpacity;

            if (group.hasStroke && !g->hasStroke)
            {
                g->hasStroke = true;
                g->strokeColor = group.strokeColor;
            }

            if (group.hasFill && !g->hasFill)
            {
                g->hasFill = true;
                g->fillColor = group.fillColor;
            }

            if (group.hasStrokeWidth && !g->hasStrokeWidth)
            {
                g->hasStrokeWidth = true;
                g->strokeWidth = group.strokeWidth;
            }

            if (group.hasStrokeOpacity && !g->hasStrokeOpacity)
            {
                g->hasStrokeOpacity = true;
                g->strokeOpacity = group.strokeOpacity;
            }

            if (group.hasFillOpacity && !g->hasFillOpacity)
            {
                g->hasFillOpacity = true;
                g->fillOpacity = group.fillOpacity;
            }

            g->Draw(*this);

            // restore
            g->hasStroke = old_hasStroke;
            g->strokeColor = old_strokeColor;
            g->hasFill = old_hasFill;
            g->fillColor = old_fillColor;
            g->hasStrokeWidth = old_hasStrokeWidth;
            g->strokeWidth = old_strokeWidth;
            g->hasStrokeOpacity = old_hasStrokeOpacity;
            g->strokeOpacity = old_strokeOpacity;
            g->hasFillOpacity = old_hasFillOpacity;
            g->fillOpacity = old_fillOpacity;

            continue;
        }


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
            tmp.fillColor = computeFillColor(text->fillColor);
            tmp.strokeColor = computeStrokeColor(text->strokeColor);
            if (group.hasStrokeWidth)
                tmp.strokeWidth = text->strokeWidth * group.strokeWidth;
            DrawText(tmp);
        }
        else
        {
            child->Draw(*this);
        }
    }

    graphics.Restore(state);
}

