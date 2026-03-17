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

        std::string BuildResultSubtitle(const MenuContext& context) {
            if (context.lastResult == GameResult::Draw) {
                return "Draw";
            }

            if ((context.lastResult == GameResult::XWin || context.lastResult == GameResult::OWin)
                && !context.winnerDisplayName.empty()) {
                return context.winnerDisplayName + " wins";
            }

            return GetDisplayText(context.lastResult);
        }

    } // namespace

    MenuView BuildResultMenuView(const MenuContext& context) {
        MenuView view;
        view.screen = MenuScreen::Result;
        view.title = "RESULT";
        view.subtitle = BuildResultSubtitle(context);
        view.message = context.statusMessage;
        view.footerHint = "W/S: move | Enter: confirm";

        view.items.push_back(MakeActionItem("restart", "Play Again", context.resultSelected == 0));
        view.items.push_back(MakeActionItem("save_game", "Save Game", context.resultSelected == 1));
        view.items.push_back(MakeActionItem("back_to_main", "Back to Main Menu", context.resultSelected == 2));

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