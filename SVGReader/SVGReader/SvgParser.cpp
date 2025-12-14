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
