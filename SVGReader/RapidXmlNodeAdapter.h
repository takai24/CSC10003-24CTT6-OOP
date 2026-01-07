#ifndef _RAPIDXMLNODEADAPTER_H_
#define _RAPIDXMLNODEADAPTER_H_
#include "IXMLNode.h"
#include "rapidxml.hpp" 

class RapidXmlNodeAdapter : public IXMLNode {
private:
    rapidxml::xml_node<>* m_node;

public:
    RapidXmlNodeAdapter(rapidxml::xml_node<>* node);
    ~RapidXmlNodeAdapter() override = default;

    std::string getTagName() const override;
    std::string getAttribute(const std::string& name) const override;
    std::string getTextContent() const override;
    std::vector<IXMLNodePtr> getChildren() const override;
};

#endif