#include "stdafx.h"
#include "Renderer.h"
#include <fstream>
#include <sstream>
#include <windows.h>

using namespace rapidxml;

// Constructor
SvgRenderer::SvgRenderer()
{
}

// Load file
bool SvgRenderer::Load(const std::wstring& filePath)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0)
        return false;
    
    std::vector<char> narrowPath(size_needed);
    WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, &narrowPath[0], size_needed, NULL, NULL);
    
    std::ifstream file(&narrowPath[0], std::ios::binary);
    if (!file)
        return false;

    std::stringstream ss;
    ss << file.rdbuf();
    std::string xml = ss.str();

    return ParseSVG(xml);
}

// Parse file
bool SvgRenderer::ParseSVG(const std::string& xml)
{
    lines.clear();

    std::vector<char> buffer(xml.begin(), xml.end());
    buffer.push_back('\0');

    xml_document<> doc;
    try {
        doc.parse<0>(&buffer[0]);
    }
    catch (...) {
        return false;
    }

    xml_node<>* svg = doc.first_node("svg");
    if (!svg)
        return false;

    // Parse <line> elements
    for (xml_node<>* node = svg->first_node("line"); node; node = node->next_sibling("line"))
    {
        // Check for required attributes
        xml_attribute<>* attr_x1 = node->first_attribute("x1");
        xml_attribute<>* attr_y1 = node->first_attribute("y1");
        xml_attribute<>* attr_x2 = node->first_attribute("x2");
        xml_attribute<>* attr_y2 = node->first_attribute("y2");
        
        if (!attr_x1 || !attr_y1 || !attr_x2 || !attr_y2)
            continue; // Skip line if required attributes are missing
        
        LineElement line = {};
        line.x1 = std::stof(attr_x1->value());
        line.y1 = std::stof(attr_y1->value());
        line.x2 = std::stof(attr_x2->value());
        line.y2 = std::stof(attr_y2->value());

        const char* stroke = node->first_attribute("stroke") ?
            node->first_attribute("stroke")->value() : "black";

        line.color = ParseColor(stroke);

        const char* width = node->first_attribute("stroke-width") ?
            node->first_attribute("stroke-width")->value() : "1";

        line.strokeWidth = std::stof(width);

        lines.push_back(line);
    }

    return true;
}

// Convert SVG color string → GDI+ color
// Supports formats: "red", "#RRGGBB"
Gdiplus::Color SvgRenderer::ParseColor(const std::string& value)
{
    if (value.empty())
        return Gdiplus::Color(255, 0, 0, 0); // default black
    
    if (value[0] == '#')
    {
        unsigned int r, g, b;
        if (sscanf_s(value.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3)
        {
            return Gdiplus::Color(255, static_cast<BYTE>(r), static_cast<BYTE>(g), static_cast<BYTE>(b));
        }
        return Gdiplus::Color(255, 0, 0, 0);
    }

    if (value == "red")   return Gdiplus::Color(255, 255, 0, 0);
    if (value == "blue")  return Gdiplus::Color(255, 0, 0, 255);
    if (value == "green") return Gdiplus::Color(255, 0, 255, 0);
    if (value == "black") return Gdiplus::Color(255, 0, 0, 0);

    return Gdiplus::Color(255, 0, 0, 0); // default black (just for sure)
}

// Draw loaded elements
void SvgRenderer::Draw(Graphics& g)
{
    for (const auto& line : lines)
    {
        Pen pen(line.color, line.strokeWidth);
        g.DrawLine(&pen, line.x1, line.y1, line.x2, line.y2);
    }
}
