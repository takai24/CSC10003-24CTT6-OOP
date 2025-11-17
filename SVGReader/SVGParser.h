#pragma once

#include <vector>
#include <memory>
#include <string>

#include "IXMLNode.h"
#include "CRapidXmlNodeAdapter.h"
#include "rapidxml.hpp"

using namespace std;

class SVGElementFactory;

class SVGParser {
private:
    SVGElementFactory& m_factory;
    std::unique_ptr<rapidxml::xml_document<>> m_xmlDoc;
    std::vector<char> m_fileBuffer;

    void parseElement(IXMLNode* node);

public:
    explicit SVGParser(SVGElementFactory& factory);

    bool parse(const std::string& filename);
};
