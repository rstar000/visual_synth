#include "GridUI/GridUI.hpp"

#include <sstream>
#include <string>

ImRect TransformComponent(ImRect const& componentRect, ImRect const& contentRect)
{
    const auto& dstSize = contentRect.GetSize();
    ImRect subScaled = {componentRect.Min * dstSize, componentRect.Max * dstSize};
    subScaled.Translate(contentRect.Min);
    return subScaled;
}

ImVec2 TransformPoint(ImVec2 const& pt, ImRect const& contentRect)
{
    const auto& dstSize = contentRect.GetSize();
    ImVec2 scaled = pt * dstSize + contentRect.Min;
    return scaled;
}
