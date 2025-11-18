#include "stdafx.h"
#include "SvgElement.h"
#include "IRenderer.h"

void SvgLine::Draw(IRenderer &renderer) const
{
    renderer.DrawLine(*this);
}

void SvgRect::Draw(IRenderer &renderer) const
{
    renderer.DrawRect(*this);
}

void SvgCircle::Draw(IRenderer &renderer) const
{
    renderer.DrawCircle(*this);
}

void SvgEllipse::Draw(IRenderer &renderer) const
{
    renderer.DrawEllipse(*this);
}

void SvgPolyline::Draw(IRenderer &renderer) const
{
    renderer.DrawPolyline(*this);
}

void SvgPolygon::Draw(IRenderer &renderer) const
{
    renderer.DrawPolygon(*this);
}

void SvgText::Draw(IRenderer &renderer) const
{
    renderer.DrawText(*this);
}

void SvgGroup::Draw(IRenderer &renderer) const
{
    for (const auto &child : children)
    {
        if (child)
            child->Draw(renderer);
    }
}
