#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <vector>
using namespace std;

class IXMLNode {
public:
    virtual string getTagName() = 0;
    virtual string getAttribute(const string& name) = 0;
    virtual vector<IXMLNode*> getChildren() = 0;
    virtual ~IXMLNode() = default;
};