#include "stdafx.h"
#include "SVGDocument.h"
SVGDocument::SVGDocument(){}

void SVGDocument::addRootElement(unique_ptr<ISVGElement> element)
{
	m_rootElements.push_back(std::move(element));
}
void SVGDocument::load(const string& filename, SVGParser& parser)
{
	parser.parse(filename);


}

void SVGDocument::render(SVGRenderer* renderer)
{
	for (const auto& element : m_rootElements) {
		element->render(renderer);
	}
}

