#include "stdafx.h"
#include "SvgParser.h"
#include "SvgGradient.h"
#include "SvgLinearGradient.h"
#include "SvgRadialGradient.h"
#include "SvgColors.h"

using namespace rapidxml;

bool SvgParser::Parse(const std::string& xml, SvgDocument& document)
{
    std::vector<char> buffer(xml.begin(), xml.end());
    buffer.push_back('\0');

    xml_document<> doc;
    try
    {
        doc.parse<0>(&buffer[0]);
    }
    catch (...)
    {
        return false;
    }

    xml_node<>* svg = doc.first_node("svg");
    if (!svg)
        return false;

    RapidXmlNodeAdapter root(svg);
    // Extract width/height or viewBox from root svg element if present
    auto parseDimension = [](const std::string& s) -> float
        {
            if (s.empty())
                return 0.0f;
            // take numeric prefix (handles values like "800", "800px", "800.5")
            size_t i = 0;
            // skip leading spaces
            while (i < s.size() && isspace(static_cast<unsigned char>(s[i])))
                ++i;
            size_t start = i;
            // accept sign
            if (i < s.size() && (s[i] == '+' || s[i] == '-'))
                ++i;
            bool seenDigit = false;
            while (i < s.size() && (isdigit(static_cast<unsigned char>(s[i]))))
            {
                ++i;
                seenDigit = true;
            }
            if (i < s.size() && s[i] == '.')
            {
                ++i;
                while (i < s.size() && isdigit(static_cast<unsigned char>(s[i])))
                {
                    ++i;
                    seenDigit = true;
                }
            }
            if (!seenDigit)
                return 0.0f;
            try
            {
                return std::stof(s.substr(start, i - start));
            }
            catch (...)
            {
                return 0.0f;
            }
        };

    std::string wattr = root.getAttribute("width");
    std::string hattr = root.getAttribute("height");
    std::string vb = root.getAttribute("viewBox");
    if (!wattr.empty() && !hattr.empty())
    {
        float w = parseDimension(wattr);
        float h = parseDimension(hattr);
        if (w > 0 && h > 0)
            document.SetSize(w, h);
    }
    else if (!vb.empty())
    {
        // viewBox: minx miny width height
        // split by spaces or commas
        std::vector<float> nums;
        std::string tmp = vb;
        for (char& c : tmp)
            if (c == ',')
                c = ' ';
        std::stringstream ss(tmp);
        float v;
        while (ss >> v)
            nums.push_back(v);
        if (nums.size() >= 4)
        {
            document.SetSize(nums[2], nums[3]);
        }
    }
    ParseChildren(root, document, nullptr);
    return true;
}

static float ParseOffset(const std::string& s) {
    if (s.empty()) return 0.0f;
    if (s.back() == '%') return std::stof(s) / 100.0f;
    return std::stof(s);
}
static float AttrFloat(const IXMLNode& node, const std::string& name, float def) {
    std::string val = node.getAttribute(name);
    if (val.empty()) return def;
    if (val.back() == '%') return std::stof(val) / 100.0f;
    return std::stof(val);
}

void SvgParser::ParseChildren(const IXMLNode& parent, SvgDocument& document, SvgGroup* currentGroup)
{
    auto children = parent.getChildren();
    for (auto& childPtr : children)
    {
        IXMLNode& child = *childPtr;
        const std::string tag = child.getTagName();

        if (tag == "defs")
        {
            auto defChildren = child.getChildren();
            for (auto& defPtr : defChildren)
            {
                IXMLNode& defNode = *defPtr;
                std::string defTag = defNode.getTagName();

                if (defTag == "linearGradient")
                {
                    auto grad = std::make_shared<SvgLinearGradient>();
                    grad->id = defNode.getAttribute("id");

                    if (!defNode.getAttribute("gradientUnits").empty())
                        grad->gradientUnits = defNode.getAttribute("gradientUnits");

                    if (!defNode.getAttribute("gradientTransform").empty())
                        grad->gradientTransform = defNode.getAttribute("gradientTransform");

                    std::string href = defNode.getAttribute("xlink:href");
                    if (href.empty()) href = defNode.getAttribute("href");
                    if (!href.empty() && href[0] == '#') grad->href = href.substr(1);
                    if (!defNode.getAttribute("x1").empty()) grad->x1 = AttrFloat(defNode, "x1", 0.0f);
                    if (!defNode.getAttribute("y1").empty()) grad->y1 = AttrFloat(defNode, "y1", 0.0f);
                    if (!defNode.getAttribute("x2").empty()) grad->x2 = AttrFloat(defNode, "x2", 1.0f); // Default 100%
                    if (!defNode.getAttribute("y2").empty()) grad->y2 = AttrFloat(defNode, "y2", 0.0f);

                    auto stops = defNode.getChildren();
                    for (auto& stopPtr : stops)
                    {
                        IXMLNode& stopNode = *stopPtr;
                        if (stopNode.getTagName() != "stop") continue;

                        float offset = ParseOffset(stopNode.getAttribute("offset"));
                        std::string colorStr = stopNode.getAttribute("stop-color");
                        std::string opacityStr = stopNode.getAttribute("stop-opacity");

                        Gdiplus::Color finalColor = SvgColors::GetColor(colorStr, opacityStr);
                        grad->stops.push_back(SvgGradientStop(offset, finalColor));
                    }

                    if (!grad->id.empty()) document.gradients[grad->id] = grad;
                }

                else if (defTag == "radialGradient")
                {
                    auto grad = std::make_shared<SvgRadialGradient>();
                    grad->id = defNode.getAttribute("id");

                    if (!defNode.getAttribute("gradientUnits").empty())
                        grad->gradientUnits = defNode.getAttribute("gradientUnits");

                    if (!defNode.getAttribute("gradientTransform").empty())
                        grad->gradientTransform = defNode.getAttribute("gradientTransform");

                    std::string href = defNode.getAttribute("xlink:href");
                    if (href.empty()) href = defNode.getAttribute("href");
                    if (!href.empty() && href[0] == '#') grad->href = href.substr(1);

                    if (!defNode.getAttribute("cx").empty()) grad->cx = AttrFloat(defNode, "cx", 0.5f);
                    if (!defNode.getAttribute("cy").empty()) grad->cy = AttrFloat(defNode, "cy", 0.5f);
                    if (!defNode.getAttribute("r").empty())  grad->r = AttrFloat(defNode, "r", 0.5f);

                    if (!defNode.getAttribute("fx").empty()) grad->fx = AttrFloat(defNode, "fx", grad->cx);
                    else grad->fx = grad->cx;

                    if (!defNode.getAttribute("fy").empty()) grad->fy = AttrFloat(defNode, "fy", grad->cy);
                    else grad->fy = grad->cy;

                    auto stops = defNode.getChildren();
                    for (auto& stopPtr : stops)
                    {
                        IXMLNode& stopNode = *stopPtr;
                        if (stopNode.getTagName() != "stop") continue;

                        float offset = ParseOffset(stopNode.getAttribute("offset"));
                        std::string colorStr = stopNode.getAttribute("stop-color");
                        std::string opacityStr = stopNode.getAttribute("stop-opacity");

                        Gdiplus::Color finalColor = SvgColors::GetColor(colorStr, opacityStr);
                        grad->stops.push_back(SvgGradientStop(offset, finalColor));
                    }

                    if (!grad->id.empty()) document.gradients[grad->id] = grad;
                }
            }
            continue;
        }

        if (tag == "g")
        {
            auto group = std::make_unique<SvgGroup>();
            SvgGroup* groupRaw = group.get();

            if (!child.getAttribute("transform").empty())
            {
                group->transformAttribute = child.getAttribute("transform");
            }

            if (!child.getAttribute("stroke").empty())
            {
                group->hasStroke = true;
                group->strokeColor = factory.ParseColor(child.getAttribute("stroke"));
            }

            if (!child.getAttribute("fill").empty())
            {
                group->hasFill = true;
                group->fillColor = factory.ParseColor(child.getAttribute("fill"));
            }

            if (!child.getAttribute("stroke-width").empty())
            {
                group->hasStrokeWidth = true;
                group->strokeWidth = std::stof(child.getAttribute("stroke-width"));
            }

            if (!child.getAttribute("stroke-opacity").empty())
            {
                group->hasStrokeOpacity = true;
                group->strokeOpacity = std::stof(child.getAttribute("stroke-opacity"));
            }

            if (!child.getAttribute("fill-opacity").empty())
            {
                group->hasFillOpacity = true;
                group->fillOpacity = std::stof(child.getAttribute("fill-opacity"));
            }

            if (currentGroup)
            {
                currentGroup->AddChild(std::move(group));
            }
            else
            {
                document.AddElement(std::move(group));
            }
            ParseChildren(child, document, groupRaw);
        }
        else
        {
            auto element = factory.CreateElement(child);
            if (element)
            {
                if (currentGroup)
                {
                    currentGroup->AddChild(std::move(element));
                }
                else
                {
                    document.AddElement(std::move(element));
                }
            }
        }
    }
}