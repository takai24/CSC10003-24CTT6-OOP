#pragma once

#include <gdiplus.h>
#include <string>
#include <vector>
#include <memory>

using Gdiplus::Color;
using Gdiplus::PointF;

class IRenderer;

class ISvgElement
{
public:
    virtual ~ISvgElement() {}
    std::string transformAttribute;
    virtual void Draw(IRenderer &renderer) const = 0;

    bool hasInputFill = false;
    bool hasInputStroke = false;
    bool hasInputStrokeWidth = false;
    bool hasInputStrokeOpacity = false;
    bool hasInputFillOpacity = false;
};

class ISvgShape : public ISvgElement
{
public:
    virtual ~ISvgShape() {}
};

class SvgLine : public ISvgShape
{
public:
    float x1{}, y1{}, x2{}, y2{};
    Color strokeColor{255, 0, 0, 0};
    float strokeWidth{1.0f};

    void Draw(IRenderer &renderer) const override;
};

class SvgRect : public ISvgShape
{
public:
    float x{}, y{}, w{}, h{};
    Color fillColor{0, 0, 0, 0};
    Color strokeColor{255, 0, 0, 0};
    float strokeWidth{1.0f};

    void Draw(IRenderer &renderer) const override;
};

class SvgCircle : public ISvgShape
{
public:
    float cx{}, cy{}, r{};
    Color fillColor{0, 0, 0, 0};
    Color strokeColor{255, 0, 0, 0};
    float strokeWidth{1.0f};

    void Draw(IRenderer &renderer) const override;
};

class SvgEllipse : public ISvgShape
{
public:
    float cx{}, cy{}, rx{}, ry{};
    Color fillColor{0, 0, 0, 0};
    Color strokeColor{255, 0, 0, 0};
    float strokeWidth{1.0f};

    void Draw(IRenderer &renderer) const override;
};

class SvgPolyline : public ISvgShape
{
public:
    std::vector<PointF> points;
    Color strokeColor{255, 0, 0, 0};
    float strokeWidth{1.0f};
    Gdiplus::Color fillColor;
    void Draw(IRenderer &renderer) const override;
};

class SvgPolygon : public ISvgShape
{
public:
    std::vector<PointF> points;
    Color fillColor{0, 0, 0, 0};
    Color strokeColor{255, 0, 0, 0};
    float strokeWidth{1.0f};

    void Draw(IRenderer &renderer) const override;
};

class SvgText : public ISvgElement
{
public:
    float x{}, y{};
    std::wstring text;
    std::wstring fontFamily{L"Arial"};
    float fontSize{16.0f};
    Color fillColor{255, 0, 0, 0};
    std::string textAnchor = "start";
    Color strokeColor{ 0, 0, 0, 0 };
    float strokeWidth{ 1.0f };
    void Draw(IRenderer &renderer) const override;
};

class SvgGroup : public ISvgElement
{
public:
    Gdiplus::Color strokeColor;
    Gdiplus::Color fillColor;
    float strokeWidth = 1.0f;
    float strokeOpacity = 1.0f;
    float fillOpacity = 1.0f;

    std::vector<std::unique_ptr<ISvgElement>> children;

    void AddChild(std::unique_ptr<ISvgElement> child)
    {
        children.push_back(std::move(child));
    }

    void Draw(IRenderer &renderer) const override;
};

class SvgPath : public ISvgElement
{
public:
    std::unique_ptr<Gdiplus::GraphicsPath> pathData;

    Gdiplus::Color fillColor;
    Gdiplus::Color strokeColor;
    float strokeWidth;

    SvgPath() = default;

    // Custom copy constructor to allow copying SvgPath (deep copy of pathData)
    SvgPath(const SvgPath &other)
        : fillColor(other.fillColor),
          strokeColor(other.strokeColor),
          strokeWidth(other.strokeWidth)
    {
        if (other.pathData)
        {
            // GraphicsPath copy ctor is protected; create a new path and copy contents
            pathData = std::make_unique<Gdiplus::GraphicsPath>();
            pathData->AddPath(other.pathData.get(), FALSE);
        }
        transformAttribute = other.transformAttribute;
    }

    SvgPath &operator=(const SvgPath &other)
    {
        if (this != &other)
        {
            fillColor = other.fillColor;
            strokeColor = other.strokeColor;
            strokeWidth = other.strokeWidth;
            transformAttribute = other.transformAttribute;
            if (other.pathData)
            {
                pathData = std::make_unique<Gdiplus::GraphicsPath>();
                pathData->AddPath(other.pathData.get(), FALSE);
            }
            else
            {
                pathData.reset();
            }
        }
        return *this;
    }

    void Draw(IRenderer &renderer) const override;
};
