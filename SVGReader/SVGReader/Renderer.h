#pragma once
#include "stdafx.h"

using namespace std;
using namespace rapidxml;
using namespace Gdiplus;

class SvgRenderer
{
    private:
        struct LineElement
        {
            float x1, y1, x2, y2;
            Color color;
            float strokeWidth;
        };
        vector<LineElement> lines;

        struct RectElement
        {
            float x, y, w, h;
            Color fillColor;
            Color strokeColor;
            float strokeWidth;
        };
	    vector<RectElement> rects;

        struct CircleElement
        {
            float cx, cy, r;
            Color fillColor;
            Color strokeColor;
            float strokeWidth;
        };
	    vector<CircleElement> circles;

        struct EllipseElement
        {
            float cx, cy, rx, ry;
            Color fillColor;
            Color strokeColor;
            float strokeWidth;
        };
	    vector<EllipseElement> ellipses;

        struct PolylineElement
        {
            vector<PointF> points;
            Color strokeColor;
            float strokeWidth;
        };
	    vector<PolylineElement> polylines;

        struct PolygonElement
        {
            vector<PointF> points;
            Color fillColor;
            Color strokeColor;
            float strokeWidth;
        };
	    vector<PolygonElement> polygons;

    public:
        SvgRenderer();

        bool ParseSVG(const string& xml);
        Color ParseColor(const string& value);

        bool Load(const wstring& filePath);
        void Draw(Graphics& g);
        void Clear();
};