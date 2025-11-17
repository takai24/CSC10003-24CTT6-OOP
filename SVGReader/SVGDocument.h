#pragma once
#include "stdafx.h"
#include <vector>
#include <memory>
#include <string>
using namespace std;

class ISVGElement; // Forward declaration
class SVGRenderer;   // Forward declaration
class SVGParser;  // Forward declaration (nếu load() gọi trực tiếp parser)

class SVGDocument {
private:
    vector<unique_ptr<ISVGElement>> m_rootElements;
    float m_width = 0.0f;
    float m_height = 0.0f;

public:
    SVGDocument();
    // Phương thức để thêm một phần tử gốc (parser sẽ gọi)
    void addRootElement(unique_ptr<ISVGElement> element);

    // Phương thức tải file SVG (sẽ gọi SVGParser)
    void load(const string& filename, SVGParser& parser); // Truyền parser vào

    // Phương thức để vẽ toàn bộ tài liệu
    void render(SVGRenderer* renderer);

    // Các getter cho width, height
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }
    void setWidth(float w) { m_width = w; }
    void setHeight(float h) { m_height = h; }
};