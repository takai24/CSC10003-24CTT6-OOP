#include "stdafx.h"
#include "Renderer.h"
#include "SvgParser.h"

bool SvgRenderer::Load(const std::wstring &filePath)
{
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed <= 0)
        return false;

    std::vector<char> narrowPath(size_needed);
    WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, &narrowPath[0], size_needed, NULL, NULL);

    std::ifstream file(&narrowPath[0], std::ios::binary);
    if (!file)
        return false;

    std::stringstream ss;
    ss << file.rdbuf();
    std::string xml = ss.str();

    SvgParser parser;
    return parser.Parse(xml, document);
}
