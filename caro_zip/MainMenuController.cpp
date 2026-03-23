#include "MainMenuController.h"

#include "MenuNavigation.h"
#include "MenuText.h"

namespace caro {

    namespace {

        const int kMainMenuItemCount = 6;

        MenuItemView MakeItem(const std::string& id, const std::string& label, bool selected) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            return item;
        }

    } // namespace

    MenuView BuildMainMenuView(const MenuContext& context) {
        const Language lang = context.appSettings.language;

        MenuView view;
        view.screen = MenuScreen::Main;
        view.title = SelectText(lang, "MENU CHINH", "MAIN MENU");
        view.subtitle = SelectText(lang, "Menu tach rieng de UI team goi lai", "Menu separated for UI team");
        view.message = context.statusMessage;
        view.footerHint = SelectText(
            lang,
            "Mui ten / W,S,A,D / I,J,K,L: di chuyen | Enter: xac nhan",
            "Arrow / W,S,A,D / I,J,K,L: move | Enter: confirm"
        );

        view.items.push_back(MakeItem("new_game", SelectText(lang, "Choi moi", "New Game"), context.mainSelected == 0));
        view.items.push_back(MakeItem("load_game", SelectText(lang, "Tai game", "Load Game"), context.mainSelected == 1));
        view.items.push_back(MakeItem("settings", SelectText(lang, "Cai dat", "Settings"), context.mainSelected == 2));
        view.items.push_back(MakeItem("how_to_play", SelectText(lang, "Cach choi", "How to Play"), context.mainSelected == 3));
        view.items.push_back(MakeItem("about_us", SelectText(lang, "Ve chung toi", "About Us"), context.mainSelected == 4));
        view.items.push_back(MakeItem("exit", SelectText(lang, "Thoat", "Exit"), context.mainSelected == 5));

        return view;
    }

    MenuCommand HandleMainMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;

        if (input == MenuInput::Up) {
            context.mainSelected = WrapSelectionIndex(context.mainSelected, -1, kMainMenuItemCount);
            return command;
        }

        if (input == MenuInput::Down) {
            context.mainSelected = WrapSelectionIndex(context.mainSelected, 1, kMainMenuItemCount);
            return command;
        }

        if (input != MenuInput::Confirm) {
            return command;
        }

        switch (context.mainSelected) {
        case 0:
            OpenNewGameMenuScreen(context);
            break;
        case 1:
            OpenLoadMenuScreen(context);
            command.type = MenuCommandType::OpenLoadMenu;
            break;
        case 2:
            OpenSettingsMenuScreen(context, MenuScreen::Main);
            command.type = MenuCommandType::OpenSettings;
            break;
        case 3:
            OpenHowToPlayScreen(context, MenuScreen::Main);
            command.type = MenuCommandType::OpenHowToPlay;
            break;
        case 4:
            OpenAboutUsScreen(context, MenuScreen::Main);
            command.type = MenuCommandType::OpenAboutUs;
            break;
        case 5:
            command.type = MenuCommandType::ExitApplication;
            break;
        default:
            break;
        }

        return command;
    }

} // namespace caro