#pragma once

#include "stdafx.h"
#include "SvgElement.h"
#include "SvgGradient.h"
using namespace std;

class IRenderer;

class SvgDocument
{
public:
    map<string, shared_ptr<SvgGradient>> gradients;

    shared_ptr<SvgGradient> GetGradientById(const string& id) const
    {
        auto it = gradients.find(id);
        if (it != gradients.end())
            return it->second;
        return nullptr;
    }

    void AddElement(std::unique_ptr<ISvgElement> element)
    {
        elements.push_back(std::move(element));
    }

    void Render(IRenderer& renderer) const;

    void SetSize(float w, float h)
    {
        width = w;
        height = h;
    }

    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

    void ResolveGradientLinks();
private:
    std::vector<std::unique_ptr<ISvgElement>> elements;
    float width = 0.0f;
    float height = 0.0f;
};