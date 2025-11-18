#include "DrawLine.h"

// Draw loaded elements
void SvgRenderer::Draw(Graphics& g)
{
    for (const auto& line : lines)
    {
        Pen pen(line.color, line.strokeWidth);
        g.DrawLine(&pen, line.x1, line.y1, line.x2, line.y2);
    }
}
