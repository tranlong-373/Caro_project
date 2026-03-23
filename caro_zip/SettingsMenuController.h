#pragma once

#include "MenuContext.h"

namespace caro {

    MenuView BuildSettingsMenuView(const MenuContext& context);
    MenuCommand HandleSettingsMenuInput(MenuContext& context, MenuInput input);

} // namespace caro