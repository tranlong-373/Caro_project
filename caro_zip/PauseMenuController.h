#pragma once

#include "MenuContext.h"

namespace caro {

    MenuView BuildPauseMenuView(const MenuContext& context);
    MenuCommand HandlePauseMenuInput(MenuContext& context, MenuInput input);

} // namespace caro