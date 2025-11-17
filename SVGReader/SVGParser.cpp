#include "stdafx.h"
#include "SVGParser.h"
#include "SVGElementFactory.h"
#include <fstream>
#include <iostream>

using namespace std;


void SVGParser::parseElement(IXMLNode* node) {
    cout << "Parsing element: " << node->getTagName() << endl;
    m_factory.createElement(node->getTagName());
    vector<IXMLNode*> children = node->getChildren();
    for (auto* child : children) {
        parseElement(child);
        delete child;
    }
}

SVGParser::SVGParser(SVGElementFactory& factory) :
    m_factory(factory),
    m_xmlDoc(make_unique<rapidxml::xml_document<>>()) {
}

bool SVGParser::parse(const string& filename) {
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) {
        cerr << "Cannot open file: " << filename << endl;
        return false;
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    m_fileBuffer.resize(size + 1);
    if (!file.read(m_fileBuffer.data(), size)) {
        cerr << "Failed to read file: " << filename << endl;
        return false;
    }

    m_fileBuffer[size] = '\0';

    try {
        m_xmlDoc->parse<0>(m_fileBuffer.data());
    }
    catch (const rapidxml::parse_error& e) {
        cerr << "Parse error: " << e.what() << endl;
        return false;
    }

    rapidxml::xml_node<>* root_node = m_xmlDoc->first_node();
    if (!root_node) {
        cerr << "No root node found in XML" << endl;
        return false;
    }

    CRapidXmlNodeAdapter rootAdapter(root_node);
    parseElement(&rootAdapter);

    return true;
}