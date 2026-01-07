#include "stdafx.h"
#include "SvgDocument.h"
#include "IRenderer.h"
#include "GdiPlusRenderer.h"
// gradients types not required here

void SvgDocument::Render(IRenderer& renderer) const
{
    if (auto gdiRenderer = dynamic_cast<GdiPlusRenderer*>(&renderer)) {
        gdiRenderer->SetGradients(this->GetGradients());
    }

    for (const auto& element : elements) {
        element->Draw(renderer);
    }
}