#pragma once

#include "MenuContext.h"

namespace caro {

    MenuView BuildResultMenuView(const MenuContext& context);
    MenuCommand HandleResultMenuInput(MenuContext& context, MenuInput input);

} // namespace caro