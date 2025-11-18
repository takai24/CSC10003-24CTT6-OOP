#pragma once
#include "stdafx.h"

class SvgDrawLine
{
private:
    struct LineElement
    {
        float x1, y1, x2, y2;
        Color color;
        float strokeWidth;
    };
	vector<LineElement> lines;
public:
    void Draw(Graphics& g);
};