#include "NewGameMenuController.h"

#include "MenuNavigation.h"
#include "MenuText.h"

namespace caro {

    namespace {

        const int kNewGameItemCount = 6;

        std::string GetDefaultPlayer1Name() {
            return "Player 1";
        }

        std::string GetDefaultPlayer2Name() {
            return "Player 2";
        }

        std::string GetBotName(AIDifficulty difficulty) {
            return "BOT - " + GetDisplayText(difficulty);
        }

        MenuItemView MakeChoiceItem(
            const std::string& id,
            const std::string& label,
            const std::string& value,
            bool selected,
            bool enabled = true
        ) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.value = value;
            item.kind = MenuItemKind::Choice;
            item.selected = selected;
            item.enabled = enabled;
            return item;
        }

        MenuItemView MakeTextItem(
            const std::string& id,
            const std::string& label,
            const std::string& value,
            bool selected
        ) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.value = value;
            item.kind = MenuItemKind::Text;
            item.selected = selected;
            return item;
        }

        MenuItemView MakeActionItem(const std::string& id, const std::string& label, bool selected) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            return item;
        }

        void UpdateOpponentByMode(NewGameDraft& draft) {
            if (draft.settings.gameMode == GameMode::PVE) {
                draft.playerOName = GetBotName(draft.settings.aiDifficulty);
            }
            else {
                draft.playerOName = GetDefaultPlayer2Name();
            }
        }

        void CycleGameMode(NewGameDraft& draft) {
            draft.settings.gameMode = (draft.settings.gameMode == GameMode::PVP)
                ? GameMode::PVE
                : GameMode::PVP;

            UpdateOpponentByMode(draft);
        }

        void CycleRuleMode(NewGameDraft& draft) {
            draft.settings.ruleMode = (draft.settings.ruleMode == RuleMode::FreeStyle)
                ? RuleMode::Standard
                : RuleMode::FreeStyle;
        }

        void CycleDifficulty(NewGameDraft& draft, int direction) {
            int value = static_cast<int>(draft.settings.aiDifficulty);
            value = WrapSelectionIndex(value, direction, 4);
            draft.settings.aiDifficulty = static_cast<AIDifficulty>(value);
            UpdateOpponentByMode(draft);
        }

    } // namespace

    MenuView BuildNewGameMenuView(const MenuContext& context) {
        const bool isPve = (context.newGameDraft.settings.gameMode == GameMode::PVE);

        MenuView view;
        view.screen = MenuScreen::NewGame;
        view.title = "NEW GAME";
        view.subtitle = "Select mode, rule, AI level and player names";
        view.message = context.statusMessage;
        view.footerHint = "W/S: move | A/D: change value | Enter: confirm | ESC: back";

        view.items.push_back(MakeChoiceItem("game_mode", "Game Mode", GetDisplayText(context.newGameDraft.settings.gameMode), context.newGameSelected == 0));
        view.items.push_back(MakeChoiceItem("rule_mode", "Rule Mode", GetDisplayText(context.newGameDraft.settings.ruleMode), context.newGameSelected == 1));
        view.items.push_back(MakeChoiceItem(
            "ai_level",
            "AI Level",
            isPve ? GetDisplayText(context.newGameDraft.settings.aiDifficulty) : "N/A",
            context.newGameSelected == 2,
            isPve
        ));
        view.items.push_back(MakeTextItem("player_x", "Player 1", context.newGameDraft.playerXName, context.newGameSelected == 3));
        view.items.push_back(MakeTextItem("player_o", isPve ? "BOT" : "Player 2", context.newGameDraft.playerOName, context.newGameSelected == 4));
        view.items.push_back(MakeActionItem("start_game", "Start Game", context.newGameSelected == 5));

        return view;
    }

    MenuCommand HandleNewGameMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;

        if (input == MenuInput::Up) {
            context.newGameSelected = WrapSelectionIndex(context.newGameSelected, -1, kNewGameItemCount);
            return command;
        }

        if (input == MenuInput::Down) {
            context.newGameSelected = WrapSelectionIndex(context.newGameSelected, 1, kNewGameItemCount);
            return command;
        }

        if (input == MenuInput::Left) {
            if (context.newGameSelected == 0) CycleGameMode(context.newGameDraft);
            else if (context.newGameSelected == 1) CycleRuleMode(context.newGameDraft);
            else if (context.newGameSelected == 2 && context.newGameDraft.settings.gameMode == GameMode::PVE) {
                CycleDifficulty(context.newGameDraft, -1);
            }
            return command;
        }

        if (input == MenuInput::Right) {
            if (context.newGameSelected == 0) CycleGameMode(context.newGameDraft);
            else if (context.newGameSelected == 1) CycleRuleMode(context.newGameDraft);
            else if (context.newGameSelected == 2 && context.newGameDraft.settings.gameMode == GameMode::PVE) {
                CycleDifficulty(context.newGameDraft, 1);
            }
            return command;
        }

        if (input == MenuInput::Back) {
            OpenMainMenuScreen(context);
            return command;
        }

        if (input == MenuInput::Confirm && context.newGameSelected == 5) {
            context.appSettings = context.newGameDraft.settings;

            command.type = MenuCommandType::StartNewGame;
            command.settings = context.newGameDraft.settings;
            command.text = context.newGameDraft.playerXName.empty() ? GetDefaultPlayer1Name() : context.newGameDraft.playerXName;
            command.extraText = (context.newGameDraft.settings.gameMode == GameMode::PVE)
                ? GetBotName(context.newGameDraft.settings.aiDifficulty)
                : (context.newGameDraft.playerOName.empty() ? GetDefaultPlayer2Name() : context.newGameDraft.playerOName);
            return command;
        }

        return command;
    }

} // namespace caro