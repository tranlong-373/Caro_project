#pragma once

#include "MenuContext.h"

namespace caro {

    MenuView BuildNewGameMenuView(const MenuContext& context);
    MenuCommand HandleNewGameMenuInput(MenuContext& context, MenuInput input);

} // namespace caro