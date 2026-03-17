#include "InforMenuController.h"

namespace caro {

    namespace {

        MenuItemView MakeBackItem(bool selected) {
            MenuItemView item;
            item.id = "back";
            item.label = "Back";
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            return item;
        }

        void GoBackFromInfo(MenuContext& context) {
            context.currentScreen = context.infoReturnScreen;
        }

    } // namespace

    MenuView BuildHowToPlayView(const MenuContext&) {
        MenuView view;
        view.screen = MenuScreen::HowToPlay;
        view.title = "HOW TO PLAY";
        view.subtitle = "Control: W/A/S/D to move, Enter to confirm.";
        view.message =
            "Roadmap note:\n"
            "- Use W/A/S/D to move the cursor.\n"
            "- Use Enter to place a piece or confirm a menu item.\n"
            "- FreeStyle: any 5 in a row wins.\n"
            "- Standard: 5 in a row blocked at both ends does not count.";
        view.footerHint = "Enter / ESC: back";
        view.items.push_back(MakeBackItem(true));
        return view;
    }

    MenuView BuildAboutUsView(const MenuContext&) {
        MenuView view;
        view.screen = MenuScreen::AboutUs;
        view.title = "ABOUT US";
        view.subtitle = "This screen is separated so UI team can skin it later.";
        view.message =
            "Suggested content:\n"
            "- Team name\n"
            "- Members and roles\n"
            "- Asset credits\n"
            "- Tech stack: engine, UI library, sound library";
        view.footerHint = "Enter / ESC: back";
        view.items.push_back(MakeBackItem(true));
        return view;
    }

    MenuCommand HandleInfoMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;
        if (input == MenuInput::Back || input == MenuInput::Confirm) {
            GoBackFromInfo(context);
        }
        return command;
    }

} // namespace caro