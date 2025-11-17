#pragma once
#include <iostream>
#include "IXMLNode.h"
using namespace std;

class ISVGElement {
protected:
	string m_id;
	float m_opacity = 1.0f;
	string m_transform;
private:
	virtual void parse(IXMLNode* node) = 0;
	virtual void draw(SVGRenderer& renderer) = 0;
	void parseCommonAttributes(IXMLNode* node);
	void applyCommonStyle(SVGRenderer& renderer) const;
};
