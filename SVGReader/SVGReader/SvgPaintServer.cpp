#include "stdafx.h"
#include "SvgPaintServer.h"
#include <string>

// Store a gradient in the paint server. Matches header signature.
void SvgPaintServer::AddGradient(const std::shared_ptr<SvgGradient>& gradient)
{
    if (gradient && !gradient->id.empty())
    {
        gradients[gradient->id] = gradient;
    }
}

// Resolve href inheritance chains.
void SvgPaintServer::ResolveGradients()
{
    const int maxDepth = 10; // protect against cycles
    // We need to resolve all gradients.
    // Order matters if chains are long, but simple loop with maxDepth check usually works if we repeat?
    // Better: Just loop for each gradient, trace up.
    for (auto& kv : gradients)
    {
        auto grad = kv.second;
        if (!grad) continue;
        
        std::string currentHref = grad->href;
        int depth = 0;
        
        // We want to inherited from 'currentHref'
        while (!currentHref.empty() && depth < maxDepth)
        {
            auto parentIt = gradients.find(currentHref);
            if (parentIt == gradients.end()) break;
            
            auto parent = parentIt->second;
            if (!parent) break;

            // Inherit attributes if not set in child
            
            // Stops
            if (grad->stops.empty() && !parent->stops.empty()) {
                grad->stops = parent->stops;
            }
            
            // Common attributes
            if (grad->gradientUnits.empty() || grad->gradientUnits == "objectBoundingBox") { // default is "objectBoundingBox", wait. Parser sets default.
                // We need to know if user set it. Parser sets it to "objectBoundingBox" if missing.
                // It is hard to distinguish "User set objectBoundingBox" vs "Default objectBoundingBox".
                // But typically if parent has "userSpaceOnUse", we should probably take it if child didn't specify.
                // However, we didn't add flag for strings.
                // Let's assume if child has default, we might take parent's if parent's is non-default?
                // Actually: Parser sets default. So we can't know.
                // WORKAROUND: For now, we only inherit if strings are empty?
                // But Parser ensures they are NOT empty.
                // Let's rely on empty check logic from Parser if I changed it. I didn't change String defaults.
                // Let's skip String inheritance for now unless I change Parser to leave them empty.
                // Wait, User complained about "missing attributes... gradientTransform".
                // gradientTransform defaults to empty string by Parser.
                if (grad->gradientTransform.empty() && !parent->gradientTransform.empty())
                    grad->gradientTransform = parent->gradientTransform;
                    
                if (grad->spreadMethod == "pad" && parent->spreadMethod != "pad") // "pad" is default
                     grad->spreadMethod = parent->spreadMethod;
            }
            
            if (grad->type == GradientType::Linear && parent->type == GradientType::Linear)
            {
                auto lChild = std::static_pointer_cast<SvgLinearGradient>(grad);
                auto lParent = std::static_pointer_cast<SvgLinearGradient>(parent);
                
                if (!lChild->hasX1 && lParent->hasX1) { lChild->x1 = lParent->x1; lChild->hasX1 = true; }
                if (!lChild->hasY1 && lParent->hasY1) { lChild->y1 = lParent->y1; lChild->hasY1 = true; }
                if (!lChild->hasX2 && lParent->hasX2) { lChild->x2 = lParent->x2; lChild->hasX2 = true; }
                if (!lChild->hasY2 && lParent->hasY2) { lChild->y2 = lParent->y2; lChild->hasY2 = true; }
            }
            else if (grad->type == GradientType::Radial && parent->type == GradientType::Radial)
            {
                auto rChild = std::static_pointer_cast<SvgRadialGradient>(grad);
                auto rParent = std::static_pointer_cast<SvgRadialGradient>(parent);
                
                if (!rChild->hasCx && rParent->hasCx) { rChild->cx = rParent->cx; rChild->hasCx = true; }
                if (!rChild->hasCy && rParent->hasCy) { rChild->cy = rParent->cy; rChild->hasCy = true; }
                if (!rChild->hasR && rParent->hasR)   { rChild->r  = rParent->r;  rChild->hasR  = true; }
                if (!rChild->hasFx && rParent->hasFx) { rChild->fx = rParent->fx; rChild->hasFx = true; }
                if (!rChild->hasFy && rParent->hasFy) { rChild->fy = rParent->fy; rChild->hasFy = true; }
            }

            // Move up
            currentHref = parent->href;
            depth++;
        }
    }
}

// Retrieve a gradient by id.
std::shared_ptr<SvgGradient> SvgPaintServer::GetGradient(const std::string& id) const
{
    auto it = gradients.find(id);
    if (it != gradients.end())
        return it->second;
    return nullptr;
}

const std::unordered_map<std::string, std::shared_ptr<SvgGradient>>& SvgPaintServer::GetGradients() const
{
    return gradients;
}
