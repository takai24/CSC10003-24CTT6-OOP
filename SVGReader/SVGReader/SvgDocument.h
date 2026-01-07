#ifndef _SVGDOCUMENT_H_
#define _SVGDOCUMENT_H_

#include "stdafx.h"
#include "SvgElement.h"
#include "SvgGradient.h"
#include <unordered_map>
#include <memory>
#include <string>

class IRenderer;

#include "SvgPaintServer.h"

class IRenderer;

class SvgDocument
{
public:
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

    void AddGradient(std::shared_ptr<SvgGradient> gradient)
    {
        paintServer.AddGradient(gradient);
    }

    void ResolveGradients()
    {
        paintServer.ResolveGradients();
    }

    std::shared_ptr<SvgGradient> GetGradient(const std::string& id) const
    {
        return paintServer.GetGradient(id);
    }

    // For renderer access (accessing the map from Paint Server)
    const std::unordered_map<std::string, std::shared_ptr<SvgGradient>>& GetGradients() const {
        return paintServer.GetGradients();
    }

private:
    std::vector<std::unique_ptr<ISvgElement>> elements;
    SvgPaintServer paintServer;
    float width = 0.0f;
    float height = 0.0f;
};

#endif
