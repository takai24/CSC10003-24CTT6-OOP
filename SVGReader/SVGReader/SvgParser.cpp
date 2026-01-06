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
                group->strokeWidth = std::stof(child.getAttribute("stroke-width"));
            }

            if (!child.getAttribute("stroke-opacity").empty())
            {
                group->hasInputStrokeOpacity = true;
                group->strokeOpacity = std::stof(child.getAttribute("stroke-opacity"));
            }

            if (!child.getAttribute("fill-opacity").empty())
            {
                group->hasInputFillOpacity = true;
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
