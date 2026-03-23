#pragma once

#include "MenuContext.h"

namespace caro {

    MenuView BuildMainMenuView(const MenuContext& context);
    MenuCommand HandleMainMenuInput(MenuContext& context, MenuInput input);

} // namespace caro