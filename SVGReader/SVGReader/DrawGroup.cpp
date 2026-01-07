#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

void GdiPlusRenderer::DrawGroup(const SvgGroup& group)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, group.transformAttribute);

    for (const auto& child : group.children)
    {
        if (auto g = dynamic_cast<SvgGroup*>(child.get()))
        {
            bool old_hasStroke = g->hasStroke;
            Color old_strokeColor = g->strokeColor;
            bool old_hasFill = g->hasFill;
            Color old_fillColor = g->fillColor;
            std::string old_fillAttributeString = g->fillAttributeString;
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

            if (!group.fillAttributeString.empty() && g->fillAttributeString.empty()) {
                g->fillAttributeString = group.fillAttributeString;
            }

            g->Draw(*this);

            // restore
            g->hasStroke = old_hasStroke;
            g->strokeColor = old_strokeColor;
            g->hasFill = old_hasFill;
            g->fillColor = old_fillColor;
            g->fillAttributeString = old_fillAttributeString;
            g->hasStrokeWidth = old_hasStrokeWidth;
            g->strokeWidth = old_strokeWidth;
            g->hasStrokeOpacity = old_hasStrokeOpacity;
            g->strokeOpacity = old_strokeOpacity;
            g->hasFillOpacity = old_hasFillOpacity;
            g->fillOpacity = old_fillOpacity;

            continue;
        }


        if (auto line = dynamic_cast<SvgLine*>(child.get()))
        {
            SvgLine tmp = *line;
            // Apply group's stroke color only if child has no stroke
            if (!line->hasStroke && group.hasStroke)
            {
                tmp.hasStroke = true;
                tmp.strokeColor = group.strokeColor;
                if (group.hasStrokeOpacity && !line->hasStrokeOpacity) {
                    tmp.hasStrokeOpacity = true;
                    tmp.strokeOpacity = group.strokeOpacity;
                }
                if (group.hasStrokeWidth && !line->hasStrokeWidth) {
                    tmp.hasStrokeWidth = true;
                    tmp.strokeWidth = group.strokeWidth;
                }
            }
            // Apply group's stroke-opacity multiplicatively to the child's stroke alpha
            if (group.hasStrokeOpacity)
            {
                float a = tmp.strokeColor.GetAlpha() / 255.0f;
                a *= group.strokeOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.strokeColor = Color(static_cast<BYTE>(ai), tmp.strokeColor.GetR(), tmp.strokeColor.GetG(), tmp.strokeColor.GetB());
            }
            if (group.hasStrokeWidth)
                tmp.strokeWidth = line->strokeWidth * group.strokeWidth;
            if (!group.fillAttributeString.empty() && tmp.fillAttributeString.empty()) {
                tmp.fillAttributeString = group.fillAttributeString;
            }
            if (!group.fillUrl.empty() && tmp.fillUrl.empty()) {
                tmp.fillUrl = group.fillUrl;
            }

            DrawLine(tmp);
        }
        else if (auto rect = dynamic_cast<SvgRect*>(child.get()))
        {
            SvgRect tmp = *rect;
            if (!rect->hasStroke && group.hasStroke)
            {
                tmp.hasStroke = true;
                tmp.strokeColor = group.strokeColor;
                if (group.hasStrokeOpacity && !rect->hasStrokeOpacity) {
                    tmp.hasStrokeOpacity = true;
                    tmp.strokeOpacity = group.strokeOpacity;
                }
                if (group.hasStrokeWidth && !rect->hasStrokeWidth) {
                    tmp.hasStrokeWidth = true;
                    tmp.strokeWidth = group.strokeWidth;
                }
            }
            if (!rect->hasFill && group.hasFill)
            {
                tmp.hasFill = true;
                tmp.fillColor = group.fillColor;
                if (group.hasFillOpacity && !rect->hasFillOpacity) {
                    tmp.hasFillOpacity = true;
                    tmp.fillOpacity = group.fillOpacity;
                }
            }
            if (group.hasStrokeWidth)
                tmp.strokeWidth = rect->strokeWidth * group.strokeWidth;
            // Apply group's stroke-opacity multiplicatively to the child's stroke alpha
            if (group.hasStrokeOpacity)
            {
                float a = tmp.strokeColor.GetAlpha() / 255.0f;
                a *= group.strokeOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.strokeColor = Color(static_cast<BYTE>(ai), tmp.strokeColor.GetR(), tmp.strokeColor.GetG(), tmp.strokeColor.GetB());
            }
            // inherit/apply group's fill opacity multiplicatively
            if (group.hasFillOpacity && tmp.fillColor.GetAlpha() > 0)
            {
                float a = tmp.fillColor.GetAlpha() / 255.0f;
                a *= group.fillOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.fillColor = Color(static_cast<BYTE>(ai), tmp.fillColor.GetR(), tmp.fillColor.GetG(), tmp.fillColor.GetB());
            }
            if (!group.fillAttributeString.empty() && tmp.fillAttributeString.empty()) {
                tmp.fillAttributeString = group.fillAttributeString;
            }
            DrawRect(tmp);
        }
        else if (auto circle = dynamic_cast<SvgCircle*>(child.get()))
        {
            SvgCircle tmp = *circle;
            if (!circle->hasStroke && group.hasStroke)
            {
                tmp.hasStroke = true;
                tmp.strokeColor = group.strokeColor;
                if (group.hasStrokeOpacity && !circle->hasStrokeOpacity) {
                    tmp.hasStrokeOpacity = true;
                    tmp.strokeOpacity = group.strokeOpacity;
                }
                if (group.hasStrokeWidth && !circle->hasStrokeWidth) {
                    tmp.hasStrokeWidth = true;
                    tmp.strokeWidth = group.strokeWidth;
                }
            }
            if (!circle->hasFill && group.hasFill)
            {
                tmp.hasFill = true;
                tmp.fillColor = group.fillColor;
                if (group.hasFillOpacity && !circle->hasFillOpacity) {
                    tmp.hasFillOpacity = true;
                    tmp.fillOpacity = group.fillOpacity;
                }
            }
            if (group.hasStrokeWidth)
                tmp.strokeWidth = circle->strokeWidth * group.strokeWidth;
            if (group.hasStrokeOpacity)
            {
                float a = tmp.strokeColor.GetAlpha() / 255.0f;
                a *= group.strokeOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.strokeColor = Color(static_cast<BYTE>(ai), tmp.strokeColor.GetR(), tmp.strokeColor.GetG(), tmp.strokeColor.GetB());
            }
            if (group.hasFillOpacity && tmp.fillColor.GetAlpha() > 0)
            {
                float a = tmp.fillColor.GetAlpha() / 255.0f;
                a *= group.fillOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.fillColor = Color(static_cast<BYTE>(ai), tmp.fillColor.GetR(), tmp.fillColor.GetG(), tmp.fillColor.GetB());
            }
            if (!group.fillAttributeString.empty() && tmp.fillAttributeString.empty()) {
                tmp.fillAttributeString = group.fillAttributeString;
            }
            if (!group.fillUrl.empty() && tmp.fillUrl.empty()) {
                tmp.fillUrl = group.fillUrl;
            }

            DrawCircle(tmp);
        }
        else if (auto e = dynamic_cast<SvgEllipse*>(child.get()))
        {
            SvgEllipse tmp = *e;
            if (!e->hasStroke && group.hasStroke)
            {
                tmp.hasStroke = true;
                tmp.strokeColor = group.strokeColor;
                if (group.hasStrokeOpacity && !e->hasStrokeOpacity) {
                    tmp.hasStrokeOpacity = true;
                    tmp.strokeOpacity = group.strokeOpacity;
                }
                if (group.hasStrokeWidth && !e->hasStrokeWidth) {
                    tmp.hasStrokeWidth = true;
                    tmp.strokeWidth = group.strokeWidth;
                }
            }
            if (!e->hasFill && group.hasFill)
            {
                tmp.hasFill = true;
                tmp.fillColor = group.fillColor;
                if (group.hasFillOpacity && !e->hasFillOpacity) {
                    tmp.hasFillOpacity = true;
                    tmp.fillOpacity = group.fillOpacity;
                }
            }
            if (group.hasStrokeWidth)
                tmp.strokeWidth = e->strokeWidth * group.strokeWidth;
            if (group.hasStrokeOpacity)
            {
                float a = tmp.strokeColor.GetAlpha() / 255.0f;
                a *= group.strokeOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.strokeColor = Color(static_cast<BYTE>(ai), tmp.strokeColor.GetR(), tmp.strokeColor.GetG(), tmp.strokeColor.GetB());
            }
            if (group.hasFillOpacity && tmp.fillColor.GetAlpha() > 0)
            {
                float a = tmp.fillColor.GetAlpha() / 255.0f;
                a *= group.fillOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.fillColor = Color(static_cast<BYTE>(ai), tmp.fillColor.GetR(), tmp.fillColor.GetG(), tmp.fillColor.GetB());
            }
            if (!group.fillAttributeString.empty() && tmp.fillAttributeString.empty()) {
                tmp.fillAttributeString = group.fillAttributeString;
            }
            if (!group.fillUrl.empty() && tmp.fillUrl.empty()) {
                tmp.fillUrl = group.fillUrl;
            }

            DrawEllipse(tmp);
        }
        else if (auto pl = dynamic_cast<SvgPolyline*>(child.get()))
        {
            SvgPolyline tmp = *pl;
            if (!pl->hasStroke && group.hasStroke)
            {
                tmp.hasStroke = true;
                tmp.strokeColor = group.strokeColor;
                if (group.hasStrokeOpacity && !pl->hasStrokeOpacity) {
                    tmp.hasStrokeOpacity = true;
                    tmp.strokeOpacity = group.strokeOpacity;
                }
                if (group.hasStrokeWidth && !pl->hasStrokeWidth) {
                    tmp.hasStrokeWidth = true;
                    tmp.strokeWidth = group.strokeWidth;
                }
            }
            if (!pl->hasFill && group.hasFill)
            {
                tmp.hasFill = true;
                tmp.fillColor = group.fillColor;
                if (group.hasFillOpacity && !pl->hasFillOpacity) {
                    tmp.hasFillOpacity = true;
                    tmp.fillOpacity = group.fillOpacity;
                }
            }
            if (group.hasStrokeWidth)
                tmp.strokeWidth = pl->strokeWidth * group.strokeWidth;
            if (group.hasStrokeOpacity)
            {
                float a = tmp.strokeColor.GetAlpha() / 255.0f;
                a *= group.strokeOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.strokeColor = Color(static_cast<BYTE>(ai), tmp.strokeColor.GetR(), tmp.strokeColor.GetG(), tmp.strokeColor.GetB());
            }
            if (group.hasFillOpacity && tmp.fillColor.GetAlpha() > 0)
            {
                float a = tmp.fillColor.GetAlpha() / 255.0f;
                a *= group.fillOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.fillColor = Color(static_cast<BYTE>(ai), tmp.fillColor.GetR(), tmp.fillColor.GetG(), tmp.fillColor.GetB());
            }
            if (!group.fillAttributeString.empty() && tmp.fillAttributeString.empty()) {
                tmp.fillAttributeString = group.fillAttributeString;
            }
            if (!group.fillUrl.empty() && tmp.fillUrl.empty()) {
                tmp.fillUrl = group.fillUrl;
            }

            DrawPolyline(tmp);
        }
        else if (auto pg = dynamic_cast<SvgPolygon*>(child.get()))
        {
            SvgPolygon tmp = *pg;
            if (!pg->hasStroke && group.hasStroke)
            {
                tmp.hasStroke = true;
                tmp.strokeColor = group.strokeColor;
                if (group.hasStrokeOpacity && !pg->hasStrokeOpacity) {
                    tmp.hasStrokeOpacity = true;
                    tmp.strokeOpacity = group.strokeOpacity;
                }
                if (group.hasStrokeWidth && !pg->hasStrokeWidth) {
                    tmp.hasStrokeWidth = true;
                    tmp.strokeWidth = group.strokeWidth;
                }
            }
            if (!pg->hasFill && group.hasFill)
            {
                tmp.hasFill = true;
                tmp.fillColor = group.fillColor;
                if (group.hasFillOpacity && !pg->hasFillOpacity) {
                    tmp.hasFillOpacity = true;
                    tmp.fillOpacity = group.fillOpacity;
                }
            }
            if (group.hasStrokeWidth)
                tmp.strokeWidth = pg->strokeWidth * group.strokeWidth;
            if (group.hasStrokeOpacity)
            {
                float a = tmp.strokeColor.GetAlpha() / 255.0f;
                a *= group.strokeOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.strokeColor = Color(static_cast<BYTE>(ai), tmp.strokeColor.GetR(), tmp.strokeColor.GetG(), tmp.strokeColor.GetB());
            }
            if (group.hasFillOpacity && tmp.fillColor.GetAlpha() > 0)
            {
                float a = tmp.fillColor.GetAlpha() / 255.0f;
                a *= group.fillOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.fillColor = Color(static_cast<BYTE>(ai), tmp.fillColor.GetR(), tmp.fillColor.GetG(), tmp.fillColor.GetB());
            }
            if (!group.fillAttributeString.empty() && tmp.fillAttributeString.empty()) {
                tmp.fillAttributeString = group.fillAttributeString;
            }
            DrawPolygon(tmp);
        }
        else if (auto path = dynamic_cast<SvgPath*>(child.get()))
        {
            SvgPath tmp = *path;
            if (!path->hasStroke && group.hasStroke)
            {
                tmp.hasStroke = true;
                tmp.strokeColor = group.strokeColor;
                if (group.hasStrokeOpacity && !path->hasStrokeOpacity) {
                    tmp.hasStrokeOpacity = true;
                    tmp.strokeOpacity = group.strokeOpacity;
                }
                if (group.hasStrokeWidth && !path->hasStrokeWidth) {
                    tmp.hasStrokeWidth = true;
                    tmp.strokeWidth = group.strokeWidth;
                }
            }
            if (!path->hasFill && group.hasFill)
            {
                tmp.hasFill = true;
                tmp.fillColor = group.fillColor;
                if (group.hasFillOpacity && !path->hasFillOpacity) {
                    tmp.hasFillOpacity = true;
                    tmp.fillOpacity = group.fillOpacity;
                }
            }
            if (group.hasStrokeWidth)
                tmp.strokeWidth = path->strokeWidth * group.strokeWidth;
            if (group.hasStrokeOpacity)
            {
                float a = tmp.strokeColor.GetAlpha() / 255.0f;
                a *= group.strokeOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.strokeColor = Color(static_cast<BYTE>(ai), tmp.strokeColor.GetR(), tmp.strokeColor.GetG(), tmp.strokeColor.GetB());
            }
            if (group.hasFillOpacity && tmp.fillColor.GetAlpha() > 0)
            {
                float a = tmp.fillColor.GetAlpha() / 255.0f;
                a *= group.fillOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.fillColor = Color(static_cast<BYTE>(ai), tmp.fillColor.GetR(), tmp.fillColor.GetG(), tmp.fillColor.GetB());
            }
            if (!group.fillAttributeString.empty() && tmp.fillAttributeString.empty()) {
                tmp.fillAttributeString = group.fillAttributeString;
            }
            if (!group.fillUrl.empty() && tmp.fillUrl.empty()) {
                tmp.fillUrl = group.fillUrl;
            }

            DrawPath(tmp);
        }
        else if (auto text = dynamic_cast<SvgText*>(child.get()))
        {
            SvgText tmp = *text;
            if (!text->hasFill && group.hasFill)
            {
                tmp.hasFill = true;
                tmp.fillColor = group.fillColor;
                if (group.hasFillOpacity && !text->hasFillOpacity) {
                    tmp.hasFillOpacity = true;
                    tmp.fillOpacity = group.fillOpacity;
                }
            }
            if (!text->hasStroke && group.hasStroke)
            {
                tmp.hasStroke = true;
                tmp.strokeColor = group.strokeColor;
                if (group.hasStrokeOpacity && !text->hasStrokeOpacity) {
                    tmp.hasStrokeOpacity = true;
                    tmp.strokeOpacity = group.strokeOpacity;
                }
                if (group.hasStrokeWidth && !text->hasStrokeWidth) {
                    tmp.hasStrokeWidth = true;
                    tmp.strokeWidth = group.strokeWidth;
                }
            }
            if (group.hasStrokeWidth)
                tmp.strokeWidth = text->strokeWidth * group.strokeWidth;
            if (group.hasStrokeOpacity)
            {
                float a = tmp.strokeColor.GetAlpha() / 255.0f;
                a *= group.strokeOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.strokeColor = Color(static_cast<BYTE>(ai), tmp.strokeColor.GetR(), tmp.strokeColor.GetG(), tmp.strokeColor.GetB());
            }
            if (group.hasFillOpacity && tmp.fillColor.GetAlpha() > 0)
            {
                float a = tmp.fillColor.GetAlpha() / 255.0f;
                a *= group.fillOpacity;
                int ai = static_cast<int>(a * 255.0f + 0.5f);
                ai = (ai < 0) ? 0 : (ai > 255 ? 255 : ai);
                tmp.fillColor = Color(static_cast<BYTE>(ai), tmp.fillColor.GetR(), tmp.fillColor.GetG(), tmp.fillColor.GetB());
            }
            if (!group.fillAttributeString.empty() && tmp.fillAttributeString.empty()) {
                tmp.fillAttributeString = group.fillAttributeString;
            }
            if (!group.fillUrl.empty() && tmp.fillUrl.empty()) {
                tmp.fillUrl = group.fillUrl;
            }

            DrawText(tmp);
        }
        else
        {
            child->Draw(*this);
        }
    }

    graphics.Restore(state);
}
