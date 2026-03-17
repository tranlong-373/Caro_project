#pragma once

#include "MenuContext.h"

namespace caro {

    MenuView BuildLoadMenuView(const MenuContext& context);
    MenuCommand HandleLoadMenuInput(MenuContext& context, MenuInput input);

} // namespace caro