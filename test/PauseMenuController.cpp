#include "PauseMenuController.h"

#include "MenuNavigation.h"
#include "MenuText.h"

namespace caro {

    namespace {

        const int kPauseMenuItemCount = 5;

        MenuItemView MakeActionItem(const std::string& id, const std::string& label, bool selected, bool enabled = true) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            item.enabled = enabled;
            return item;
        }

    } // namespace

    MenuView BuildPauseMenuView(const MenuContext& context) {
        const Language lang = context.appSettings.language;

        MenuView view;
        view.screen = MenuScreen::Pause;
        view.title = SelectText(lang, "MENU TAM DUNG", "PAUSE MENU");
        view.subtitle = SelectText(lang, "Tiep tuc, luu, cai dat, choi lai hoac thoat", "Continue, save, settings, restart or exit");
        view.message = context.statusMessage;
        view.footerHint = SelectText(lang, "W/S: di chuyen | Enter: xac nhan | ESC: tiep tuc", "W/S: move | Enter: confirm | ESC: continue");

        view.items.push_back(MakeActionItem("continue", SelectText(lang, "Tiep tuc", "Continue"), context.pauseSelected == 0));
        view.items.push_back(MakeActionItem("save_game", SelectText(lang, "Luu game", "Save Game"), context.pauseSelected == 1, context.canSaveCurrentGame));
        view.items.push_back(MakeActionItem("settings", SelectText(lang, "Cai dat", "Settings"), context.pauseSelected == 2));
        view.items.push_back(MakeActionItem("restart", SelectText(lang, "Choi lai", "Restart"), context.pauseSelected == 3));
        view.items.push_back(MakeActionItem("back_to_main", SelectText(lang, "Ve menu chinh", "Back to Main Menu"), context.pauseSelected == 4));

        return view;
    }

    MenuCommand HandlePauseMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;

        if (input == MenuInput::Up) {
            context.pauseSelected = WrapSelectionIndex(context.pauseSelected, -1, kPauseMenuItemCount);
            return command;
        }

        if (input == MenuInput::Down) {
            context.pauseSelected = WrapSelectionIndex(context.pauseSelected, 1, kPauseMenuItemCount);
            return command;
        }

        if (input == MenuInput::Back) {
            command.type = MenuCommandType::ContinueGame;
            return command;
        }

        if (input != MenuInput::Confirm) {
            return command;
        }

        switch (context.pauseSelected) {
        case 0:
            command.type = MenuCommandType::ContinueGame;
            break;
        case 1:
            if (context.canSaveCurrentGame) {
                OpenSaveMenuScreen(context, context.saveNameDraft);
                command.type = MenuCommandType::OpenSaveMenu;
            }
            else {
                SetMenuStatusMessage(
                    context,
                    SelectText(context.appSettings.language, "Trang thai hien tai khong the luu.", "Current state cannot be saved.")
                );
            }
            break;
        case 2:
            OpenSettingsMenuScreen(context, MenuScreen::Pause);
            command.type = MenuCommandType::OpenSettings;
            break;
        case 3:
            command.type = MenuCommandType::RestartGame;
            break;
        case 4:
            command.type = MenuCommandType::BackToMainMenu;
            break;
        default:
            break;
        }

        return command;
    }

} // namespace caro