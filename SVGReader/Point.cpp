#include "stdafx.h"

#include "Point.h"

Point::Point() {}

Point::Point(float x, float y): x(x), y(y) {}

Point::Point(const Point& point): x(point.x), y(point.y) {}