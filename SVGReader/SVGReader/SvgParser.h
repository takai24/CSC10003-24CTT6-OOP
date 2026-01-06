#pragma once

#include "stdafx.h"
#include "SvgDocument.h"
#include "SvgElementFactory.h"
#include "IXMLNode.h"
#include "RapidXmlNodeAdapter.h"

class SvgParser
{
public:
    SvgParser() = default;

    bool Parse(const std::string &xml, SvgDocument &document);

private:
    SvgElementFactory factory;

    void ParseChildren(const IXMLNode &parent, SvgDocument &document, SvgGroup *currentGroup);
};
