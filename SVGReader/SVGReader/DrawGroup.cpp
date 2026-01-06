#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawGroup(const SvgGroup &group)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, group.transformAttribute);
    auto computeStrokeColor = [&](const Color &childStroke, bool childHasStroke) -> Color
    {
        Color base = (childHasStroke) ? childStroke : (group.hasInputStroke ? group.strokeColor : childStroke);
        float baseA = base.GetAlpha() / 255.0f;
        float childA = childStroke.GetAlpha() / 255.0f;
        if (group.hasInputStroke && childStroke.GetAlpha() == 0 && !childHasStroke)
            childA = 1.0f;
        float groupA = group.hasInputStrokeOpacity ? group.strokeOpacity : 1.0f;
        float finalA = baseA * childA * groupA;
        int aInt = static_cast<int>(finalA * 255.0f + 0.5f);
        if (aInt < 0)
            aInt = 0;
        if (aInt > 255)
            aInt = 255;
        BYTE a = static_cast<BYTE>(aInt);
        return Color(a, base.GetR(), base.GetG(), base.GetB());
    };

    auto computeFillColor = [&](const Color &childFill, bool childHasFill) -> Color
    {
        Color base = (childHasFill) ? childFill : (group.hasInputFill ? group.fillColor : childFill);
        float baseA = base.GetAlpha() / 255.0f;
        float childA = childFill.GetAlpha() / 255.0f;
        float groupA = group.hasInputFillOpacity ? group.fillOpacity : 1.0f;
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
            bool old_hasStroke = g->hasInputStroke;
            Color old_strokeColor = g->strokeColor;
            bool old_hasFill = g->hasInputFill;
            Color old_fillColor = g->fillColor;
            bool old_hasStrokeWidth = g->hasInputStrokeWidth;
            float old_strokeWidth = g->strokeWidth;
            bool old_hasStrokeOpacity = g->hasInputStrokeOpacity;
            float old_strokeOpacity = g->strokeOpacity;
            bool old_hasFillOpacity = g->hasInputFillOpacity;
            float old_fillOpacity = g->fillOpacity;

            if (group.hasInputStroke && !g->hasInputStroke)
            {
                g->hasInputStroke = true;
                g->strokeColor = group.strokeColor;
            }

            if (group.hasInputFill && !g->hasInputFill)
            {
                g->hasInputFill = true;
                g->fillColor = group.fillColor;
            }

            if (group.hasInputStrokeWidth && !g->hasInputStrokeWidth)
            {
                g->hasInputStrokeWidth = true;
                g->strokeWidth = group.strokeWidth;
            }

            if (group.hasInputStrokeOpacity && !g->hasInputStrokeOpacity)
            {
                g->hasInputStrokeOpacity = true;
                g->strokeOpacity = group.strokeOpacity;
            }

            if (group.hasInputFillOpacity && !g->hasInputFillOpacity)
            {
                g->hasInputFillOpacity = true;
                g->fillOpacity = group.fillOpacity;
            }

            g->Draw(*this);

            // restore
            g->hasInputStroke = old_hasStroke;
            g->strokeColor = old_strokeColor;
            g->hasInputFill = old_hasFill;
            g->fillColor = old_fillColor;
            g->hasInputStrokeWidth = old_hasStrokeWidth;
            g->strokeWidth = old_strokeWidth;
            g->hasInputStrokeOpacity = old_hasStrokeOpacity;
            g->strokeOpacity = old_strokeOpacity;
            g->hasInputFillOpacity = old_hasFillOpacity;
            g->fillOpacity = old_fillOpacity;

            continue;
        }


        if (auto line = dynamic_cast<SvgLine *>(child.get()))
        {
            SvgLine tmp = *line;
            tmp.strokeColor = computeStrokeColor(line->strokeColor, line->hasInputStroke);
            if (group.hasInputStrokeWidth && !line->hasInputStrokeWidth)
                tmp.strokeWidth = line->strokeWidth * group.strokeWidth;
            else if (group.hasInputStrokeWidth && line->hasInputStrokeWidth)
                 tmp.strokeWidth = line->strokeWidth; // If child has stroke width, do we multiply? Usually replacing, or multiplying if relative? SVG spec says inherit.
                 // Actually stroke-width is not inherited by multiplication usually, it's just inherited if not specified.
                 // But here the code was doing multiplication? 
                 // The old code: if (group.hasStrokeWidth) tmp.strokeWidth = line->strokeWidth * group.strokeWidth;
                 // Wait, line->strokeWidth is initialized to 1.0f. If not specified, it is 1.0.
                 // If group specifies 4, and line doesn't specify, line should be 4?
                 // If line specifies 2, line should be 2.
                 // The multiplication logic seems suspicious or specific to this engine's interpretation.
                 // For now, I will stick to "if child has input, use child. if not, use group".
                 // BUT, I'll keep the multiplication if it was intended for scaling?
                 // No, transform handles scaling. This is likely just wrong inheritance logic in the old code or I'm misunderstanding.
                 // Given the task is about color, I should be careful with stroke width changes.
                 // However, "fix the code to render correct".
                 // Let's assume standard inheritance: if child not set, use parent.
            if (group.hasInputStrokeWidth && !line->hasInputStrokeWidth)
                 tmp.strokeWidth = group.strokeWidth;
            
            DrawLine(tmp);
        }
        else if (auto rect = dynamic_cast<SvgRect *>(child.get()))
        {
            SvgRect tmp = *rect;
            tmp.strokeColor = computeStrokeColor(rect->strokeColor, rect->hasInputStroke);
            tmp.fillColor = computeFillColor(rect->fillColor, rect->hasInputFill);
            if (group.hasInputStrokeWidth && !rect->hasInputStrokeWidth)
                tmp.strokeWidth = group.strokeWidth;
            DrawRect(tmp);
        }
        else if (auto circle = dynamic_cast<SvgCircle *>(child.get()))
        {
            SvgCircle tmp = *circle;
            tmp.strokeColor = computeStrokeColor(circle->strokeColor, circle->hasInputStroke);
            tmp.fillColor = computeFillColor(circle->fillColor, circle->hasInputFill);
            if (group.hasInputStrokeWidth && !circle->hasInputStrokeWidth)
                tmp.strokeWidth = group.strokeWidth;
            DrawCircle(tmp);
        }
        else if (auto e = dynamic_cast<SvgEllipse *>(child.get()))
        {
            SvgEllipse tmp = *e;
            tmp.strokeColor = computeStrokeColor(e->strokeColor, e->hasInputStroke);
            tmp.fillColor = computeFillColor(e->fillColor, e->hasInputFill);
            if (group.hasInputStrokeWidth && !e->hasInputStrokeWidth)
                tmp.strokeWidth = group.strokeWidth;
            DrawEllipse(tmp);
        }
        else if (auto pl = dynamic_cast<SvgPolyline *>(child.get()))
        {
            SvgPolyline tmp = *pl;
            tmp.strokeColor = computeStrokeColor(pl->strokeColor, pl->hasInputStroke);
            tmp.fillColor = computeFillColor(pl->fillColor, pl->hasInputFill);
            if (group.hasInputStrokeWidth && !pl->hasInputStrokeWidth)
                tmp.strokeWidth = group.strokeWidth;
            DrawPolyline(tmp);
        }
        else if (auto pg = dynamic_cast<SvgPolygon *>(child.get()))
        {
            SvgPolygon tmp = *pg;
            tmp.strokeColor = computeStrokeColor(pg->strokeColor, pg->hasInputStroke);
            tmp.fillColor = computeFillColor(pg->fillColor, pg->hasInputFill);
            if (group.hasInputStrokeWidth && !pg->hasInputStrokeWidth)
                tmp.strokeWidth = group.strokeWidth;
            DrawPolygon(tmp);
        }
        else if (auto path = dynamic_cast<SvgPath *>(child.get()))
        {
            SvgPath tmp = *path;
            tmp.strokeColor = computeStrokeColor(path->strokeColor, path->hasInputStroke);
            tmp.fillColor = computeFillColor(path->fillColor, path->hasInputFill);
            if (group.hasInputStrokeWidth && !path->hasInputStrokeWidth)
                tmp.strokeWidth = group.strokeWidth;
            DrawPath(tmp);
        }
        else if (auto text = dynamic_cast<SvgText *>(child.get()))
        {
            SvgText tmp = *text;
            tmp.fillColor = computeFillColor(text->fillColor, text->hasInputFill);
            tmp.strokeColor = computeStrokeColor(text->strokeColor, text->hasInputStroke);
            if (group.hasInputStrokeWidth && !text->hasInputStrokeWidth)
                tmp.strokeWidth = group.strokeWidth;
            DrawText(tmp);
        }
        else
        {
            child->Draw(*this);
        }
    }

    graphics.Restore(state);
}

