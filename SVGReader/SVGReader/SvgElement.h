#pragma once

#include "stdafx.h"

using namespace Gdiplus;

class IRenderer;

class ISvgElement
{
public:
    virtual ~ISvgElement() {}
    virtual void Draw(IRenderer &renderer) const = 0;
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

    void Draw(IRenderer &renderer) const override;
};

class SvgGroup : public ISvgElement
{
public:
    std::vector<std::unique_ptr<ISvgElement>> children;

    void AddChild(std::unique_ptr<ISvgElement> child)
    {
        children.push_back(std::move(child));
    }

    void Draw(IRenderer &renderer) const override;
};
