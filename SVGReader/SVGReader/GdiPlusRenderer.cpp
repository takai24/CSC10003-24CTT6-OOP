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
        child->Draw(*this); 
    }
    graphics.Restore(state);
}