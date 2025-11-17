#include "stdafx.h"
#include "CRapidXmlNodeAdapter.h"

CRapidXmlNodeAdapter::CRapidXmlNodeAdapter(rapidxml::xml_node<>* node) : m_node(node) {}

string CRapidXmlNodeAdapter::getTagName()  {
    if (!m_node)
        return "";
    return string(m_node->name(), m_node->name_size());
}

string CRapidXmlNodeAdapter::getAttribute(const string& name)  {
    if (!m_node)
        return "";
    auto* attr = m_node->first_attribute(name.c_str());
    if (!attr)
        return "";
    return string(attr->value(), attr->value_size());
}

vector<IXMLNode*> CRapidXmlNodeAdapter::getChildren()  {
    vector<IXMLNode*> children;

    for (auto* child = m_node->first_node(); child; child = child->next_sibling()) {
        children.push_back(new CRapidXmlNodeAdapter(child));
    }

    return children;
}
