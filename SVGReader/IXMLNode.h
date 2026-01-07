#ifndef _IXMLNODE_H_
#define _IXMLNODE_H_

#include <string>
#include <vector>
#include <memory>

class IXMLNode;

using IXMLNodePtr = std::unique_ptr<IXMLNode>;

class IXMLNode
{
public:
    virtual ~IXMLNode() = default;

    virtual std::string getTagName() const = 0;
    virtual std::string getAttribute(const std::string &name) const = 0;
    virtual std::string getTextContent() const = 0;
    virtual std::vector<IXMLNodePtr> getChildren() const = 0;
};

#endif