#include "stdafx.h"
#include "SvgParser.h"

#include "SvgGradient.h"
#include <regex>

using namespace rapidxml;

namespace {
    std::string AttrOr(const IXMLNode &node, const char *name, const std::string &def = "")
    {
        std::string v = node.getAttribute(name);
        return v.empty() ? def : v;
    }

    float ParseFloat(const std::string& str)
    {
        if (str.empty()) return 0.0f;
        std::stringstream ss(str);
        ss.imbue(std::locale::classic()); // Force C locale (dot decimal)
        float f = 0.0f;
        ss >> f;
        return f;
    }

    float ParseFloatOrPercentage(const std::string& s, float def)
    {
         if (s.empty()) return def;
         try {
             if (s.back() == '%') {
                 return ParseFloat(s.substr(0, s.size() - 1)) / 100.0f;
             }
             return ParseFloat(s);
         } catch(...) { return def; }
    }

    float AttrOrFloatPercentage(const IXMLNode &node, const char *name, float def)
    {
        return ParseFloatOrPercentage(node.getAttribute(name), def);
    }
    std::unordered_map<std::string, std::string> ParseStyleAttribute(const std::string& style)
    {
        std::unordered_map<std::string, std::string> styles;
        if (style.empty()) return styles;

        std::stringstream ss(style);
        std::string item;
        while (std::getline(ss, item, ';'))
        {
            size_t colon = item.find(':');
            if (colon != std::string::npos)
            {
                std::string key = item.substr(0, colon);
                std::string val = item.substr(colon + 1);
                
                // trim spaces
                auto trim = [](std::string& s) {
                    size_t first = s.find_first_not_of(' ');
                    if (std::string::npos == first) { s = ""; return; }
                    size_t last = s.find_last_not_of(' ');
                    s = s.substr(first, (last - first + 1));
                };
                trim(key);
                trim(val);
                if (!key.empty()) styles[key] = val;
            }
        }
        return styles;
    }
}

void SvgParser::ParseGradientStops(const IXMLNode &node, SvgGradient *grad)
{
     auto children = node.getChildren();
     for (auto &c : children)
     {
         if (c->getTagName() == "stop")
         {
             GradientStop stop;
             std::string offsetStr = c->getAttribute("offset");
             stop.offset = ParseFloatOrPercentage(offsetStr, 0.0f);
             
             std::string styleStr = c->getAttribute("style");
             auto styles = ParseStyleAttribute(styleStr);

             auto getAttr = [&](const char* name) {
                 if (styles.count(name)) return styles[name];
                 return c->getAttribute(name);
             };

             std::string colorStr = getAttr("stop-color");
             std::string opacityStr = getAttr("stop-opacity");
             
             // We need factory to parse color, but factory is member of SvgParser.
             // Accessing factory from here.
             stop.color = factory.ParseColor(colorStr);
             
             if (!opacityStr.empty())
             {
                 float op = 1.0f;
                 op = ParseFloat(opacityStr);
                 // Apply opacity
                 if (op < 1.0f)
                 {
                     if (op < 0.0f) op = 0.0f;
                     float a = stop.color.GetAlpha() / 255.0f;
                     a *= op;
                     stop.color = Gdiplus::Color(static_cast<BYTE>(a * 255.5f), stop.color.GetR(), stop.color.GetG(), stop.color.GetB());
                 }
             }
             
             grad->stops.push_back(stop);
         }
     }
}

void SvgParser::ParseGradient(const IXMLNode &child, SvgDocument &document)
{
    const std::string tag = child.getTagName();
    if (tag == "linearGradient")
    {
        auto grad = std::make_shared<SvgLinearGradient>();
        grad->id = child.getAttribute("id");
        grad->gradientUnits = AttrOr(child, "gradientUnits", "objectBoundingBox");
        grad->gradientTransform = AttrOr(child, "gradientTransform", "");
        grad->spreadMethod = AttrOr(child, "spreadMethod", "pad");

        // Parse xlink:href for gradient inheritance
        std::string href = AttrOr(child, "href", "");
        if (href.empty()) href = AttrOr(child, "xlink:href", "");
        if (!href.empty() && href[0] == '#') href = href.substr(1);
        grad->href = href;

        if (AttrOr(child, "x1", "") != "") {
             grad->x1 = AttrOrFloatPercentage(child, "x1", 0.0f);
             grad->hasX1 = true;
        }
        if (AttrOr(child, "y1", "") != "") {
             grad->y1 = AttrOrFloatPercentage(child, "y1", 0.0f);
             grad->hasY1 = true;
        }
        if (AttrOr(child, "x2", "") != "") {
             grad->x2 = AttrOrFloatPercentage(child, "x2", 1.0f);
             grad->hasX2 = true;
        }
        if (AttrOr(child, "y2", "") != "") {
             grad->y2 = AttrOrFloatPercentage(child, "y2", 0.0f);
             grad->hasY2 = true;
        }

        ParseGradientStops(child, grad.get());
        document.AddGradient(grad);
    }
    else if (tag == "radialGradient")
    {
        auto grad = std::make_shared<SvgRadialGradient>();
        grad->id = child.getAttribute("id");
        grad->gradientUnits = AttrOr(child, "gradientUnits", "objectBoundingBox");
        grad->gradientTransform = AttrOr(child, "gradientTransform", "");
        grad->spreadMethod = AttrOr(child, "spreadMethod", "pad");

        // Parse xlink:href for gradient inheritance
        std::string href = AttrOr(child, "href", "");
        if (href.empty()) href = AttrOr(child, "xlink:href", "");
        if (!href.empty() && href[0] == '#') href = href.substr(1);
        grad->href = href;

        if (AttrOr(child, "cx", "") != "") {
             grad->cx = AttrOrFloatPercentage(child, "cx", 0.5f);
             grad->hasCx = true;
        }
        if (AttrOr(child, "cy", "") != "") {
             grad->cy = AttrOrFloatPercentage(child, "cy", 0.5f);
             grad->hasCy = true;
        }
        if (AttrOr(child, "r", "") != "") {
             grad->r = AttrOrFloatPercentage(child, "r", 0.5f);
             grad->hasR = true;
        }
        if (AttrOr(child, "fx", "") != "") {
             grad->fx = AttrOrFloatPercentage(child, "fx", grad->cx);
             grad->hasFx = true;
        }
        if (AttrOr(child, "fy", "") != "") {
             grad->fy = AttrOrFloatPercentage(child, "fy", grad->cy);
             grad->hasFy = true;
        }

        ParseGradientStops(child, grad.get());
        document.AddGradient(grad);
    }
}


bool SvgParser::Parse(const std::string &xml, SvgDocument &document)
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

    xml_node<> *svg = doc.first_node("svg");
    if (!svg)
        return false;

    RapidXmlNodeAdapter root(svg);
    // Extract width/height or viewBox from root svg element if present
    // Extract width/height or viewBox from root svg element if present
    // Use the robust ParseFloat instead of the manual lambda
    auto parseDimension = [](const std::string &s) -> float
    {
        if (s.empty()) return 0.0f;
        std::stringstream ss(s);
        ss.imbue(std::locale::classic());
        float val = 0.0f;
        ss >> val;
        return val;
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
        for (char &c : tmp)
            if (c == ',')
                c = ' ';
        std::stringstream ss(tmp);
        ss.imbue(std::locale::classic());
        float v;
        while (ss >> v)
            nums.push_back(v);
        if (nums.size() >= 4)
        {
            document.SetSize(nums[2], nums[3]);
        }
    }
    ParseChildren(root, document, nullptr);
    document.ResolveGradients();
    return true;
}

void SvgParser::ParseChildren(const IXMLNode &parent, SvgDocument &document, SvgGroup *currentGroup)
{
    auto children = parent.getChildren();
    for (auto &childPtr : children)
    {
        IXMLNode &child = *childPtr;
        const std::string tag = child.getTagName();

        if (tag == "linearGradient" || tag == "radialGradient")
        {
            ParseGradient(child, document);
        }
        else if (tag == "defs")
        {
             auto defChildren = child.getChildren();
             for (auto &dc : defChildren)
             {
                 std::string dTag = dc->getTagName();
                 if (dTag == "linearGradient" || dTag == "radialGradient")
                 {
                     ParseGradient(*dc, document);
                 }
                 // If we support symbols or other defs later, handle here
             }
        }
        else if (tag == "g")
        {
            auto group = std::make_unique<SvgGroup>();
            SvgGroup *groupRaw = group.get();

            if (!child.getAttribute("transform").empty())
            {
                group->transformAttribute = child.getAttribute("transform");
            }

            if (!child.getAttribute("stroke").empty())
            {
                group->hasInputStroke = true;
                group->strokeColor = factory.ParseColor(child.getAttribute("stroke"));
            }

            if (!child.getAttribute("fill").empty())
            {
                group->hasInputFill = true;
                group->fillColor = factory.ParseColor(child.getAttribute("fill"));
            }

            if (!child.getAttribute("stroke-width").empty())
            {
                group->hasInputStrokeWidth = true;
                group->strokeWidth = ParseFloat(child.getAttribute("stroke-width"));
            }

            if (!child.getAttribute("stroke-opacity").empty())
            {
                group->hasInputStrokeOpacity = true;
                group->strokeOpacity = ParseFloat(child.getAttribute("stroke-opacity"));
            }

            if (!child.getAttribute("fill-opacity").empty())
            {
                group->hasInputFillOpacity = true;
                group->fillOpacity = ParseFloat(child.getAttribute("fill-opacity"));
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
