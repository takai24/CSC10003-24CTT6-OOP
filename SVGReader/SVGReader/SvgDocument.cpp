#include "stdafx.h"
#include "SvgDocument.h"
#include "IRenderer.h"
#include "GdiPlusRenderer.h"
#include "SvgLinearGradient.h"
#include "SvgRadialGradient.h"

void SvgDocument::ResolveGradientLinks()
{
    for (auto& pair : gradients) {
        auto grad = pair.second;

        if (grad->stops.empty() && !grad->href.empty()) {
            auto it = gradients.find(grad->href);
            if (it != gradients.end()) {
                grad->stops = it->second->stops; 
            }
        }
    }
}

void SvgDocument::Render(IRenderer& renderer) const
{
    if (auto gdiRenderer = dynamic_cast<GdiPlusRenderer*>(&renderer)) {
        gdiRenderer->SetGradients(&this->gradients);
    }

    for (const auto& element : elements) {
        element->Draw(renderer);
    }
}