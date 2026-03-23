#pragma once

#include "InforMenuController.h"
#include "LoadMenuController.h"
#include "MainMenuController.h"
#include "MenuContext.h"
#include "MenuSaveSlots.h"
#include "NewGameMenuController.h"
#include "PauseMenuController.h"
#include "ResultMenuController.h"
#include "SaveMenuController.h"
#include "SettingsMenuController.h"

namespace caro {

    void InitializeMenuSystem(MenuContext& context, const GameSettings& initialSettings);

    void RefreshMenuSaveSlotsFromFiles(
        MenuContext& context,
        const std::string& directoryPath = config::DEFAULT_SAVE_DIRECTORY
    );

    void UpdateMenuGameFlags(MenuContext& context, bool hasActiveGame, bool canSaveCurrentGame);
    void UpdateMenuLastResult(MenuContext& context, GameResult result);

    void UpdateMenuSaveNameDraft(MenuContext& context, const std::string& saveName);
    void UpdateMenuRenameDraft(MenuContext& context, const std::string& renameName);
    void UpdateMenuNewGameNames(MenuContext& context, const std::string& playerX, const std::string& playerO);

    void OpenPauseMenu(MenuContext& context);
    void OpenResultMenu(MenuContext& context, GameResult result);
    void OpenSaveMenu(MenuContext& context, const std::string& suggestedSaveName);

    MenuView BuildMenuView(const MenuContext& context);
    MenuCommand HandleMenuInput(MenuContext& context, MenuInput input);

} // namespace caro