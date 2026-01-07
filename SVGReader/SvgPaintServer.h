#ifndef _SVGPAINTSERVER_H_
#define _SVGPAINTSERVER_H_

#include <unordered_map>
#include <memory>
#include <string>
#include "SvgGradient.h"

/// Centralised paint‑server that stores all gradients, resolves
/// `xlink:href` inheritance and provides read‑only access for the
/// renderer.
class SvgPaintServer {
public:
    // Store a gradient definition (called from SvgParser::ParseGradient)
    void AddGradient(const std::shared_ptr<SvgGradient>& gradient);

    // Resolve `href` chains (called once after the whole document is parsed)
    void ResolveGradients();

    // Query helpers used by the renderer
    std::shared_ptr<SvgGradient> GetGradient(const std::string& id) const;
    const std::unordered_map<std::string, std::shared_ptr<SvgGradient>>& GetGradients() const;

private:
    std::unordered_map<std::string, std::shared_ptr<SvgGradient>> gradients;
};

#endif
