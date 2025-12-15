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

void GdiPlusRenderer::DrawRect(const SvgRect &rect)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, rect.transformAttribute);
    Pen pen(rect.strokeColor, rect.strokeWidth);
    SolidBrush brush(rect.fillColor);
    graphics.FillRectangle(&brush, rect.x, rect.y, rect.w, rect.h);
    graphics.DrawRectangle(&pen, rect.x, rect.y, rect.w, rect.h);
    graphics.Restore(state);
}

void GdiPlusRenderer::DrawCircle(const SvgCircle &circle)
{
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, circle.transformAttribute);
    Pen pen(circle.strokeColor, circle.strokeWidth);
    SolidBrush brush(circle.fillColor);
    float d = circle.r * 2.0f;
    graphics.FillEllipse(&brush, circle.cx - circle.r, circle.cy - circle.r, d, d);
    graphics.DrawEllipse(&pen, circle.cx - circle.r, circle.cy - circle.r, d, d);
    graphics.Restore(state);
}

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

void GdiPlusRenderer::DrawPolyline(const SvgPolyline &polyline)
{
    if (polyline.points.size() < 2)
        return;
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, polyline.transformAttribute);
    Pen pen(polyline.strokeColor, polyline.strokeWidth);
    SolidBrush brush(polyline.fillColor);
    graphics.FillPolygon(&brush, polyline.points.data(), static_cast<INT>(polyline.points.size()));
    if (polyline.strokeColor.GetAlpha() > 0)
    {
        graphics.DrawLines(&pen, polyline.points.data(), static_cast<INT>(polyline.points.size()));
    }
    graphics.Restore(state);
}

void GdiPlusRenderer::DrawPolygon(const SvgPolygon &polygon)
{
    if (polygon.points.size() < 3)
        return;
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, polygon.transformAttribute);
    Pen pen(polygon.strokeColor, polygon.strokeWidth);
    SolidBrush brush(polygon.fillColor);
    graphics.FillPolygon(&brush, polygon.points.data(), static_cast<INT>(polygon.points.size()));
    graphics.DrawPolygon(&pen, polygon.points.data(), static_cast<INT>(polygon.points.size()));
    graphics.Restore(state);
}

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

static std::vector<float> ParseNumbers(const std::string &args)
{
    std::vector<float> numbers;
    std::string temp = args;
    for (char &c : temp)
        if (c == ',')
            c = ' ';

    std::stringstream ss(temp);
    float val;
    while (ss >> val)
    {
        numbers.push_back(val);
    }
    return numbers;
}

void GdiPlusRenderer::ApplyTransform(Graphics &graphics, const std::string &transformStr)
{
    if (transformStr.empty())
        return;

    // Regex để tìm các lệnh: tên_lệnh(nội_dung)
    // Ví dụ khớp: "translate(10,20)" hoặc "rotate(45)"
    std::regex re("([a-z]+)\\s*\\(([^)]+)\\)");
    std::sregex_iterator next(transformStr.begin(), transformStr.end(), re);
    std::sregex_iterator end;

    // Duyệt qua từng lệnh theo thứ tự xuất hiện (quan trọng!)
    while (next != end)
    {
        std::smatch match = *next;
        std::string command = match[1]; // Lấy tên lệnh (translate, rotate...)
        std::string args = match[2];    // Lấy tham số bên trong ngoặc (10, 20...)

        std::vector<float> nums = ParseNumbers(args);

        if (command == "translate" && nums.size() >= 1)
        {
            float tx = nums[0];
            float ty = (nums.size() >= 2) ? nums[1] : 0.0f;
            // GDI+: Dịch chuyển trục tọa độ
            graphics.TranslateTransform(tx, ty);
        }
        else if (command == "rotate" && nums.size() >= 1)
        {
            float angle = nums[0];
            // If rotate has center parameters: rotate(angle cx cy)
            if (nums.size() >= 3)
            {
                float cx = nums[1];
                float cy = nums[2];
                // Translate to center, rotate, translate back
                graphics.TranslateTransform(cx, cy);
                graphics.RotateTransform(angle);
                graphics.TranslateTransform(-cx, -cy);
            }
            else
            {
                // GDI+: Xoay trục tọa độ (tâm xoay là gốc 0,0 hiện tại)
                graphics.RotateTransform(angle);
            }
        }
        else if (command == "scale" && nums.size() >= 1)
        {
            float sx = nums[0];
            float sy = (nums.size() >= 2) ? nums[1] : sx; // Nếu chỉ có 1 số thì scale đều (sx=sy)
            // GDI+: Co giãn trục tọa độ
            graphics.ScaleTransform(sx, sy);
        }

        next++;
    }
}

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
