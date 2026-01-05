#include "stdafx.h"
#include "SvgDrawLine.h"

// Draw loaded elements
void SvgDrawLine::Draw(Graphics& g)
{
    for (const auto& line : lines)
    {
        Pen pen(line.color, line.strokeWidth);
        g.DrawLine(&pen, line.x1, line.y1, line.x2, line.y2);
    }
}
