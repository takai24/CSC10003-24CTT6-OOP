#pragma once
#include "stdafx.h"

using namespace std;
using namespace rapidxml;
using namespace Gdiplus;

class SvgRenderer
{
public:
    SvgRenderer();
    bool Load(const wstring& filePath);
    void Draw(Graphics& g);

private:
    struct LineElement
    {
        float x1, y1, x2, y2;
        Color color;
        float strokeWidth;
    };

    vector<LineElement> lines;

    bool ParseSVG(const string& xml);
    Color ParseColor(const string& value);
};