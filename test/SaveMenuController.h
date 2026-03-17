#pragma once

#include "MenuContext.h"

namespace caro {

    MenuView BuildSaveMenuView(const MenuContext& context);
    MenuCommand HandleSaveMenuInput(MenuContext& context, MenuInput input);

} // namespace caro