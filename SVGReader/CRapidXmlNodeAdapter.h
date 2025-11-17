#pragma once

#include "stdafx.h"
#include "IXMLNode.h"
#include "rapidxml.hpp"


using namespace std;

class CRapidXmlNodeAdapter : public IXMLNode {
public:
    explicit CRapidXmlNodeAdapter(rapidxml::xml_node<>* node);

    string getTagName()  override;
    string getAttribute(const string& name)  override;
    vector<IXMLNode*> getChildren()  override;

private:
    rapidxml::xml_node<>* m_node;
};
