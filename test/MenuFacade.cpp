#include "MenuFacade.h"

namespace caro {

    void InitializeMenuSystem(MenuContext& context, const GameSettings& initialSettings) {
        InitializeMenuContext(context, initialSettings);
    }

    void RefreshMenuSaveSlotsFromFiles(MenuContext& context, const std::string& directoryPath) {
        SetMenuSaveSlots(context, BuildSaveSlotInfoList(directoryPath));
    }

    void UpdateMenuGameFlags(MenuContext& context, bool hasActiveGame, bool canSaveCurrentGame) {
        SetMenuHasActiveGame(context, hasActiveGame);
        SetMenuCanSaveCurrentGame(context, canSaveCurrentGame);
    }

    void UpdateMenuLastResult(MenuContext& context, GameResult result) {
        SetMenuLastResult(context, result);
    }

    void UpdateMenuSaveNameDraft(MenuContext& context, const std::string& saveName) {
        SetMenuSaveNameDraft(context, saveName);
    }

    void UpdateMenuRenameDraft(MenuContext& context, const std::string& renameName) {
        SetMenuRenameNameDraft(context, renameName);
    }

    void UpdateMenuNewGameNames(MenuContext& context, const std::string& playerX, const std::string& playerO) {
        SetMenuNewGameNames(context, playerX, playerO);
    }

    void OpenPauseMenu(MenuContext& context) {
        OpenPauseMenuScreen(context);
    }

    void OpenResultMenu(MenuContext& context, GameResult result) {
        OpenResultMenuScreen(context, result);
    }

    void OpenSaveMenu(MenuContext& context, const std::string& suggestedSaveName) {
        OpenSaveMenuScreen(context, suggestedSaveName);
    }

    MenuView BuildMenuView(const MenuContext& context) {
        switch (context.currentScreen) {
        case MenuScreen::Main:
            return BuildMainMenuView(context);
        case MenuScreen::NewGame:
            return BuildNewGameMenuView(context);
        case MenuScreen::Settings:
            return BuildSettingsMenuView(context);
        case MenuScreen::LoadGame:
            return BuildLoadMenuView(context);
        case MenuScreen::SaveGame:
            return BuildSaveMenuView(context);
        case MenuScreen::Pause:
            return BuildPauseMenuView(context);
        case MenuScreen::Result:
            return BuildResultMenuView(context);
        case MenuScreen::HowToPlay:
            return BuildHowToPlayView(context);
        case MenuScreen::AboutUs:
            return BuildAboutUsView(context);
        default:
            return BuildMainMenuView(context);
        }
    }

    MenuCommand HandleMenuInput(MenuContext& context, MenuInput input) {
        switch (context.currentScreen) {
        case MenuScreen::Main:
            return HandleMainMenuInput(context, input);
        case MenuScreen::NewGame:
            return HandleNewGameMenuInput(context, input);
        case MenuScreen::Settings:
            return HandleSettingsMenuInput(context, input);
        case MenuScreen::LoadGame:
            return HandleLoadMenuInput(context, input);
        case MenuScreen::SaveGame:
            return HandleSaveMenuInput(context, input);
        case MenuScreen::Pause:
            return HandlePauseMenuInput(context, input);
        case MenuScreen::Result:
            return HandleResultMenuInput(context, input);
        case MenuScreen::HowToPlay:
        case MenuScreen::AboutUs:
            return HandleInfoMenuInput(context, input);
        default:
            return MenuCommand();
        }
    }

} // namespace caro