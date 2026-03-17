#include "ResultMenuController.h"

#include "MenuNavigation.h"
#include "MenuText.h"

namespace caro {

    namespace {

        const int kResultMenuItemCount = 3;

        MenuItemView MakeActionItem(const std::string& id, const std::string& label, bool selected) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            return item;
        }

    } // namespace

    MenuView BuildResultMenuView(const MenuContext& context) {
        const Language lang = context.appSettings.language;

        MenuView view;
        view.screen = MenuScreen::Result;
        view.title = SelectText(lang, "KET QUA", "RESULT");
        view.subtitle = GetDisplayText(context.lastResult, lang);
        view.message = context.statusMessage;
        view.footerHint = SelectText(lang, "W/S: di chuyen | Enter: xac nhan", "W/S: move | Enter: confirm");

        view.items.push_back(MakeActionItem("restart", SelectText(lang, "Choi lai", "Play Again"), context.resultSelected == 0));
        view.items.push_back(MakeActionItem("save_game", SelectText(lang, "Luu game", "Save Game"), context.resultSelected == 1));
        view.items.push_back(MakeActionItem("back_to_main", SelectText(lang, "Ve menu chinh", "Back to Main Menu"), context.resultSelected == 2));

        return view;
    }

    MenuCommand HandleResultMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;

        if (input == MenuInput::Up) {
            context.resultSelected = WrapSelectionIndex(context.resultSelected, -1, kResultMenuItemCount);
            return command;
        }

        if (input == MenuInput::Down) {
            context.resultSelected = WrapSelectionIndex(context.resultSelected, 1, kResultMenuItemCount);
            return command;
        }

        if (input != MenuInput::Confirm) {
            return command;
        }

        if (context.resultSelected == 0) {
            command.type = MenuCommandType::RestartGame;
        }
        else if (context.resultSelected == 1) {
            OpenSaveMenuScreen(context, context.saveNameDraft);
            command.type = MenuCommandType::OpenSaveMenu;
        }
        else if (context.resultSelected == 2) {
            command.type = MenuCommandType::BackToMainMenu;
        }

        return command;
    }

} // namespace caro