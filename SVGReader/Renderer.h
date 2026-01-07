#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "stdafx.h"
#include "SvgDocument.h"

class SvgRenderer 
{
public:
    SvgRenderer() = default;

    bool Load(const std::wstring &filePath);

    const SvgDocument &GetDocument() const { return document; }

private:
    SvgDocument document;
};

#endif