#pragma once

#include "stdafx.h"
#include "SvgElement.h"

class IRenderer;

class SvgDocument
{
public:
    void AddElement(std::unique_ptr<ISvgElement> element)
    {
        elements.push_back(std::move(element));
    }

    void Render(IRenderer &renderer) const;

    void SetSize(float w, float h)
    {
        width = w;
        height = h;
    }
    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

private:
    std::vector<std::unique_ptr<ISvgElement>> elements;
    float width = 0.0f;
    float height = 0.0f;
};
