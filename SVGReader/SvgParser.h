#ifndef _SVGPARSER_H_
#define _SVGPARSER_H_

#include "stdafx.h"
#include "SvgDocument.h"
#include "SvgGradient.h"
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
    void ParseGradientStops(const IXMLNode &node, SvgGradient *grad);
    void ParseGradient(const IXMLNode &node, SvgDocument &document);
};

#endif
