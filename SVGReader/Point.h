#pragma once
class Point {
private:
	float x;
	float y;
public:
	Point();
	Point(float x, float y);
	Point(const Point& point);
};
