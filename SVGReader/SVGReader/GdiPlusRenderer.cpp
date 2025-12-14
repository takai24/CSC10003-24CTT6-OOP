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
    if (polyline.strokeColor.GetAlpha() > 0) {
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

static std::vector<float> ParseNumbers(const std::string& args) {
    std::vector<float> numbers;
    std::string temp = args;
    for (char& c : temp) if (c == ',') c = ' ';

    std::stringstream ss(temp);
    float val;
    while (ss >> val) {
        numbers.push_back(val);
    }
    return numbers;
}

void GdiPlusRenderer::ApplyTransform(Graphics& graphics, const std::string& transformStr)
{
    if (transformStr.empty()) return;

    // Regex để tìm các lệnh: tên_lệnh(nội_dung)
    // Ví dụ khớp: "translate(10,20)" hoặc "rotate(45)"
    std::regex re("([a-z]+)\\s*\\(([^)]+)\\)");
    std::sregex_iterator next(transformStr.begin(), transformStr.end(), re);
    std::sregex_iterator end;

    // Duyệt qua từng lệnh theo thứ tự xuất hiện (quan trọng!)
    while (next != end) {
        std::smatch match = *next;
        std::string command = match[1]; // Lấy tên lệnh (translate, rotate...)
        std::string args = match[2];    // Lấy tham số bên trong ngoặc (10, 20...)

        std::vector<float> nums = ParseNumbers(args);

        if (command == "translate" && nums.size() >= 1) {
            float tx = nums[0];
            float ty = (nums.size() >= 2) ? nums[1] : 0.0f;
            // GDI+: Dịch chuyển trục tọa độ
            graphics.TranslateTransform(tx, ty);
        }
        else if (command == "rotate" && nums.size() >= 1) {
            float angle = nums[0];
            // GDI+: Xoay trục tọa độ (tâm xoay là gốc 0,0 hiện tại)
            graphics.RotateTransform(angle);
        }
        else if (command == "scale" && nums.size() >= 1) {
            float sx = nums[0];
            float sy = (nums.size() >= 2) ? nums[1] : sx; // Nếu chỉ có 1 số thì scale đều (sx=sy)
            // GDI+: Co giãn trục tọa độ
            graphics.ScaleTransform(sx, sy);
        }

        next++;
    }
}

void GdiPlusRenderer::DrawPath(const SvgPath& path) {
    if (!path.pathData) return;

    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, path.transformAttribute);
    Pen pen(path.strokeColor, path.strokeWidth);
    SolidBrush brush(path.fillColor);
    graphics.FillPath(&brush, path.pathData.get());
    graphics.DrawPath(&pen, path.pathData.get());
    graphics.Restore(state);
}

void GdiPlusRenderer::DrawGroup(const SvgGroup& group) {
    GraphicsState state = graphics.Save();
    ApplyTransform(graphics, group.transformAttribute);

    for (const auto& child : group.children)
    {
        if (auto g = dynamic_cast<SvgGroup*>(child.get()))
        {
            g->Draw(*this);
            continue;
        }

        Color oldStrokeColor, oldFillColor;
        float oldStrokeWidth = 1.0f;

        bool hasStroke = false, hasFill = false;

        if (auto line = dynamic_cast<SvgLine*>(child.get()))
        {
            oldStrokeColor = line->strokeColor;
            oldStrokeWidth = line->strokeWidth;

            hasStroke = true;
        }
        else if (auto rect = dynamic_cast<SvgRect*>(child.get()))
        {
            oldStrokeColor = rect->strokeColor;
            oldFillColor = rect->fillColor;
            oldStrokeWidth = rect->strokeWidth;

            hasStroke = true;
            hasFill = true;
        }
        else if (auto circle = dynamic_cast<SvgCircle*>(child.get()))
        {
            oldStrokeColor = circle->strokeColor;
            oldFillColor = circle->fillColor;
            oldStrokeWidth = circle->strokeWidth;

            hasStroke = true;
            hasFill = true;
        }
        else if (auto e = dynamic_cast<SvgEllipse*>(child.get()))
        {
            oldStrokeColor = e->strokeColor;
            oldFillColor = e->fillColor;
            oldStrokeWidth = e->strokeWidth;

            hasStroke = true;
            hasFill = true;
        }
        else if (auto pl = dynamic_cast<SvgPolyline*>(child.get()))
        {
            oldStrokeColor = pl->strokeColor;
            oldFillColor = pl->fillColor;
            oldStrokeWidth = pl->strokeWidth;

            hasStroke = true;
            hasFill = true;
        }
        else if (auto pg = dynamic_cast<SvgPolygon*>(child.get()))
        {
            oldStrokeColor = pg->strokeColor;
            oldFillColor = pg->fillColor;
            oldStrokeWidth = pg->strokeWidth;

            hasStroke = true;
            hasFill = true;
        }
        else if (auto path = dynamic_cast<SvgPath*>(child.get()))
        {
            oldStrokeColor = path->strokeColor;
            oldFillColor = path->fillColor;
            oldStrokeWidth = path->strokeWidth;

            hasStroke = true;
            hasFill = true;
        }

        if (hasStroke)
        {
            auto c = (dynamic_cast<SvgLine*>(child.get()) ? dynamic_cast<SvgLine*>(child.get())->strokeColor
                : dynamic_cast<SvgRect*>(child.get()) ? dynamic_cast<SvgRect*>(child.get())->strokeColor
                : dynamic_cast<SvgCircle*>(child.get()) ? dynamic_cast<SvgCircle*>(child.get())->strokeColor
                : dynamic_cast<SvgEllipse*>(child.get()) ? dynamic_cast<SvgEllipse*>(child.get())->strokeColor
                : dynamic_cast<SvgPolyline*>(child.get()) ? dynamic_cast<SvgPolyline*>(child.get())->strokeColor
                : dynamic_cast<SvgPolygon*>(child.get()) ? dynamic_cast<SvgPolygon*>(child.get())->strokeColor
                : dynamic_cast<SvgPath*>(child.get())->strokeColor);

            if (group.hasStroke)
                c = group.strokeColor;

            if (group.hasStrokeOpacity)
            {
                c.SetValue((BYTE)(group.strokeOpacity * 255) << 24 | (c.GetValue() & 0x00FFFFFF));
            }

            if (auto line = dynamic_cast<SvgLine*>(child.get())) line->strokeColor = c;
            else if (auto rect = dynamic_cast<SvgRect*>(child.get())) rect->strokeColor = c;
            else if (auto circle = dynamic_cast<SvgCircle*>(child.get())) circle->strokeColor = c;
            else if (auto e = dynamic_cast<SvgEllipse*>(child.get())) e->strokeColor = c;
            else if (auto pl = dynamic_cast<SvgPolyline*>(child.get())) pl->strokeColor = c;
            else if (auto pg = dynamic_cast<SvgPolygon*>(child.get())) pg->strokeColor = c;
            else if (auto path = dynamic_cast<SvgPath*>(child.get())) path->strokeColor = c;
        }

        if (hasFill && group.hasFill)
        {
            Color c = group.fillColor;
            if (group.hasFillOpacity)
                c.SetValue((BYTE)(group.fillOpacity * 255) << 24 | (c.GetValue() & 0x00FFFFFF));

            if (auto rect = dynamic_cast<SvgRect*>(child.get())) rect->fillColor = c;
            else if (auto circle = dynamic_cast<SvgCircle*>(child.get())) circle->fillColor = c;
            else if (auto e = dynamic_cast<SvgEllipse*>(child.get())) e->fillColor = c;
            else if (auto pl = dynamic_cast<SvgPolyline*>(child.get())) pl->fillColor = c;
            else if (auto pg = dynamic_cast<SvgPolygon*>(child.get())) pg->fillColor = c;
            else if (auto path = dynamic_cast<SvgPath*>(child.get())) path->fillColor = c;
        }

        if (hasStroke && group.hasStrokeWidth)
        {
            if (auto line = dynamic_cast<SvgLine*>(child.get())) line->strokeWidth *= group.strokeWidth;
            else if (auto rect = dynamic_cast<SvgRect*>(child.get())) rect->strokeWidth *= group.strokeWidth;
            else if (auto circle = dynamic_cast<SvgCircle*>(child.get())) circle->strokeWidth *= group.strokeWidth;
            else if (auto e = dynamic_cast<SvgEllipse*>(child.get())) e->strokeWidth *= group.strokeWidth;
            else if (auto pl = dynamic_cast<SvgPolyline*>(child.get())) pl->strokeWidth *= group.strokeWidth;
            else if (auto pg = dynamic_cast<SvgPolygon*>(child.get())) pg->strokeWidth *= group.strokeWidth;
            else if (auto path = dynamic_cast<SvgPath*>(child.get())) path->strokeWidth *= group.strokeWidth;
        }

        child->Draw(*this);

        if (hasStroke)
        {
            if (auto line = dynamic_cast<SvgLine*>(child.get())) line->strokeColor = oldStrokeColor, line->strokeWidth = oldStrokeWidth;
            else if (auto rect = dynamic_cast<SvgRect*>(child.get())) rect->strokeColor = oldStrokeColor, rect->strokeWidth = oldStrokeWidth;
            else if (auto circle = dynamic_cast<SvgCircle*>(child.get())) circle->strokeColor = oldStrokeColor, circle->strokeWidth = oldStrokeWidth;
            else if (auto e = dynamic_cast<SvgEllipse*>(child.get())) e->strokeColor = oldStrokeColor, e->strokeWidth = oldStrokeWidth;
            else if (auto pl = dynamic_cast<SvgPolyline*>(child.get())) pl->strokeColor = oldStrokeColor, pl->strokeWidth = oldStrokeWidth;
            else if (auto pg = dynamic_cast<SvgPolygon*>(child.get())) pg->strokeColor = oldStrokeColor, pg->strokeWidth = oldStrokeWidth;
            else if (auto path = dynamic_cast<SvgPath*>(child.get())) path->strokeColor = oldStrokeColor, path->strokeWidth = oldStrokeWidth;
        }

        if (hasFill)
        {
            if (auto rect = dynamic_cast<SvgRect*>(child.get())) rect->fillColor = oldFillColor;
            else if (auto circle = dynamic_cast<SvgCircle*>(child.get())) circle->fillColor = oldFillColor;
            else if (auto e = dynamic_cast<SvgEllipse*>(child.get())) e->fillColor = oldFillColor;
            else if (auto pl = dynamic_cast<SvgPolyline*>(child.get())) pl->fillColor = oldFillColor;
            else if (auto pg = dynamic_cast<SvgPolygon*>(child.get())) pg->fillColor = oldFillColor;
            else if (auto path = dynamic_cast<SvgPath*>(child.get())) path->fillColor = oldFillColor;
        }
    }

    graphics.Restore(state);
}
