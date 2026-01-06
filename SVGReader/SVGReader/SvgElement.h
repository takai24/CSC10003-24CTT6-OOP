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

    bool hasStroke = false;
    bool hasFill = false;
    bool hasStrokeWidth = false;
    bool hasStrokeOpacity = false;
    bool hasFillOpacity = false;

    Color strokeColor{ 0, 0, 0, 0 };
    Color fillColor{ 0, 0, 0, 0 };
    float strokeWidth{ 1.0f };
    float strokeOpacity{ 1.0f };
    float fillOpacity{ 1.0f };

    std::string transformAttribute;
    std::string fillAttributeString; 

    virtual void Draw(IRenderer& renderer) const = 0;
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
    void Draw(IRenderer& renderer) const override;
};

class SvgRect : public ISvgShape
{
public:
    float x{}, y{}, w{}, h{};
    void Draw(IRenderer& renderer) const override;
};

class SvgCircle : public ISvgShape
{
public:
    float cx{}, cy{}, r{};
    void Draw(IRenderer& renderer) const override;
};

class SvgEllipse : public ISvgShape
{
public:
    float cx{}, cy{}, rx{}, ry{};
    void Draw(IRenderer& renderer) const override;
};

class SvgPolyline : public ISvgShape
{
public:
    std::vector<PointF> points;
    void Draw(IRenderer& renderer) const override;
};

class SvgPolygon : public ISvgShape
{
public:
    std::vector<PointF> points;
    void Draw(IRenderer& renderer) const override;
};

class SvgText : public ISvgElement
{
public:
    float x{}, y{};
    std::wstring text;
    std::wstring fontFamily{ L"Arial" };
    float fontSize{ 16.0f };
    std::string textAnchor = "start";
    void Draw(IRenderer& renderer) const override;
};

class SvgGroup : public ISvgElement
{
public:
    std::vector<std::unique_ptr<ISvgElement>> children;

    void AddChild(std::unique_ptr<ISvgElement> child)
    {
        children.push_back(std::move(child));
    }

    void Draw(IRenderer& renderer) const override;
};

class SvgPath : public ISvgElement
{
public:
    std::unique_ptr<Gdiplus::GraphicsPath> pathData;
    Gdiplus::FillMode fillMode = Gdiplus::FillModeAlternate;

    SvgPath() noexcept = default;

    SvgPath(const SvgPath& other) : ISvgElement(other)
    {
        fillMode = other.fillMode;
        if (other.pathData)
        {
            pathData = std::make_unique<Gdiplus::GraphicsPath>();
            pathData->AddPath(other.pathData.get(), FALSE);
        }
    }

    SvgPath& operator=(const SvgPath& other)
    {
        if (this != &other)
        {
            ISvgElement::operator=(other); 
            fillMode = other.fillMode;
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

    void Draw(IRenderer& renderer) const override;
};