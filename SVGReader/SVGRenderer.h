#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "Point.h"
using namespace std;

class SVGRenderer {
private:
	void setFillColor(string color);
	void setStrokeColor(string color);
	void setStrokeWidth(float width);
	void drawRectangle(float x, float y, float w, float h);
	void drawCircle(float cx, float cy, float r);
	void drawEllipse(float cx, float cy, float rx, float ry);
	void drawLine(float x1, float y1, float x2, float y2);
	void drawPolygon(vector<Point> points);
	void drawPolyline(vector<Point> points);
	void drawText(float x, float y, string content, string fontSize, string fontName);
	void drawPath(string pathData);
	void beginFrame();
	void endFrame();
};
