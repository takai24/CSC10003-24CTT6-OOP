#ifndef _SVGDRAWLINE_H_
#define _SVGDRAWLINE_H_
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

#endif