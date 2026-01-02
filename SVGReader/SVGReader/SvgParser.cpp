#include "stdafx.h"
#include "SvgParser.h"

using namespace rapidxml;

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
    auto parseDimension = [](const std::string &s) -> float
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
        for (char &c : tmp)
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

void SvgParser::ParseChildren(const IXMLNode &parent, SvgDocument &document, SvgGroup *currentGroup)
{
    auto children = parent.getChildren();
    for (auto &childPtr : children)
    {
        IXMLNode &child = *childPtr;
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

                    if (!defNode.getAttribute("x1").empty())
                        grad->x1 = std::stof(defNode.getAttribute("x1"));
                    if (!defNode.getAttribute("y1").empty())
                        grad->y1 = std::stof(defNode.getAttribute("y1"));
                    if (!defNode.getAttribute("x2").empty())
                        grad->x2 = std::stof(defNode.getAttribute("x2"));
                    if (!defNode.getAttribute("y2").empty())
                        grad->y2 = std::stof(defNode.getAttribute("y2"));

                    auto stops = defNode.getChildren();
                    for (auto& stopPtr : stops)
                    {
                        IXMLNode& stopNode = *stopPtr;
                        if (stopNode.getTagName() != "stop")
                            continue;

                        SvgGradientStop stop;

                        std::string offsetStr = stopNode.getAttribute("offset");
                        if (!offsetStr.empty())
                        {
                            if (offsetStr.back() == '%')
                                stop.offset = std::stof(offsetStr) / 100.0f;
                            else
                                stop.offset = std::stof(offsetStr);
                        }

                        if (!stopNode.getAttribute("stop-color").empty())
                            stop.color = stopNode.getAttribute("stop-color");

                        if (!stopNode.getAttribute("stop-opacity").empty())
                            stop.opacity = std::stof(stopNode.getAttribute("stop-opacity"));

                        grad->stops.push_back(stop);
                    }

                    if (!grad->id.empty())
                        document.gradients[grad->id] = grad;
                }

                else if (defTag == "radialGradient")
                {
                    auto grad = std::make_shared<SvgRadialGradient>();
                    grad->id = defNode.getAttribute("id");

                    if (!defNode.getAttribute("cx").empty())
                        grad->cx = std::stof(defNode.getAttribute("cx"));
                    if (!defNode.getAttribute("cy").empty())
                        grad->cy = std::stof(defNode.getAttribute("cy"));
                    if (!defNode.getAttribute("r").empty())
                        grad->r = std::stof(defNode.getAttribute("r"));
                    if (!defNode.getAttribute("fx").empty())
                        grad->fx = std::stof(defNode.getAttribute("fx"));
                    if (!defNode.getAttribute("fy").empty())
                        grad->fy = std::stof(defNode.getAttribute("fy"));

                    auto stops = defNode.getChildren();
                    for (auto& stopPtr : stops)
                    {
                        IXMLNode& stopNode = *stopPtr;
                        if (stopNode.getTagName() != "stop")
                            continue;

                        SvgGradientStop stop;

                        std::string offsetStr = stopNode.getAttribute("offset");
                        if (!offsetStr.empty())
                        {
                            if (offsetStr.back() == '%')
                                stop.offset = std::stof(offsetStr) / 100.0f;
                            else
                                stop.offset = std::stof(offsetStr);
                        }

                        if (!stopNode.getAttribute("stop-color").empty())
                            stop.color = stopNode.getAttribute("stop-color");

                        if (!stopNode.getAttribute("stop-opacity").empty())
                            stop.opacity = std::stof(stopNode.getAttribute("stop-opacity"));

                        grad->stops.push_back(stop);
                    }

                    if (!grad->id.empty())
                        document.gradients[grad->id] = grad;
                }
            }
            continue;
        }

        if (tag == "g")
        {
            auto group = std::make_unique<SvgGroup>();
            SvgGroup *groupRaw = group.get();

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
