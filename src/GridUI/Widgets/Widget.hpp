#pragma once
#include "ui/GridLayout.hpp"


struct IGridWidget
{
    /// @brief Draw widget inside the component rect
    /// @param comp target component 
    /// @param dstRect computed final target rect
    void Draw(const GridComponent& comp, ImRect dstRect);
};

/*

GRID WIDGETS:

* Knob
    * Centered
    * With range editor
* Fader (vertical)
* Combo button [A|B|C|D]
* Button toggle
* Input/Output pin
* Plot

*/