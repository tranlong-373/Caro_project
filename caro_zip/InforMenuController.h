#pragma once

#include "MenuContext.h"

namespace caro {

    MenuView BuildHowToPlayView(const MenuContext& context);
    MenuView BuildAboutUsView(const MenuContext& context);
    MenuCommand HandleInfoMenuInput(MenuContext& context, MenuInput input);

} // namespace caro