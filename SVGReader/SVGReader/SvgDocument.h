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

private:
    std::vector<std::unique_ptr<ISvgElement>> elements;
};
