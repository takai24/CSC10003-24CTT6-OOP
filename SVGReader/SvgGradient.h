#ifndef _SVGGRADIENT_H_
#define _SVGGRADIENT_H_
#include <string>
#include <vector>
#include <gdiplus.h>

enum class GradientType
{
    Linear,
    Radial
};

struct GradientStop
{
    float offset;
    Gdiplus::Color color;
};

class SvgGradient
{
public:
    virtual ~SvgGradient() = default;

    std::string id;
    GradientType type;
    std::string gradientUnits = "objectBoundingBox";
    std::string gradientTransform;
    std::string spreadMethod = "pad";
    std::string href;
    std::vector<GradientStop> stops;

protected:
    SvgGradient(GradientType t) : type(t) {}
};

class SvgRadialGradient : public SvgGradient
{
public:
    SvgRadialGradient() : SvgGradient(GradientType::Radial) {}

    float cx = 0.5f; bool hasCx = false;
    float cy = 0.5f; bool hasCy = false;
    float r = 0.5f;  bool hasR = false;
    float fx = 0.5f; bool hasFx = false;
    float fy = 0.5f; bool hasFy = false;
};

// Add check flags for Linear too
class SvgLinearGradient : public SvgGradient
{
public:
    SvgLinearGradient() : SvgGradient(GradientType::Linear) {}

    float x1 = 0.0f; bool hasX1 = false;
    float y1 = 0.0f; bool hasY1 = false;
    float x2 = 1.0f; bool hasX2 = false;
    float y2 = 0.0f; bool hasY2 = false;
};

#endif
