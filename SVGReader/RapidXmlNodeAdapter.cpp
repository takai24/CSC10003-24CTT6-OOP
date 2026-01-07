#include "stdafx.h"
#include "RapidXmlNodeAdapter.h"
#include <algorithm> 
#include <sstream>   

RapidXmlNodeAdapter::RapidXmlNodeAdapter(rapidxml::xml_node<>* node) : m_node(node) {}

std::string RapidXmlNodeAdapter::getTagName() const {
    return m_node ? m_node->name() : "";
}

std::string RapidXmlNodeAdapter::getAttribute(const std::string& name) const {
    if (!m_node) return "";
    rapidxml::xml_attribute<>* attr = m_node->first_attribute(name.c_str());
    return attr ? attr->value() : "";
}

std::string RapidXmlNodeAdapter::getTextContent() const {
    if (!m_node) return "";
    rapidxml::xml_node<>* text_node = m_node->first_node(0); 
    return text_node ? text_node->value() : "";
}

std::vector<IXMLNodePtr> RapidXmlNodeAdapter::getChildren() const {
    std::vector<IXMLNodePtr> children;
    if (!m_node) return children;

    for (rapidxml::xml_node<>* child = m_node->first_node(); child; child = child->next_sibling()) {
        if (child->type() == rapidxml::node_element) { 
            children.push_back(std::make_unique<RapidXmlNodeAdapter>(child));
        }
    }
    return children;
}