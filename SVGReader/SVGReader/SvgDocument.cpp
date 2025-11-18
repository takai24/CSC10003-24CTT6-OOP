#include "stdafx.h"
#include "SvgDocument.h"
#include "IRenderer.h"

void SvgDocument::Render(IRenderer &renderer) const
{
    for (const auto &e : elements)
    {
        if (e)
            e->Draw(renderer);
    }
}
