#pragma once

#include "GameSetting.h"
#include "MenuTypes.h"

namespace caro {

    struct MenuContext {
        MenuScreen currentScreen;
        MenuScreen previousScreen;

        MenuScreen settingsReturnScreen;
        MenuScreen saveReturnScreen;
        MenuScreen infoReturnScreen;

        GameSettings appSettings;
        NewGameDraft newGameDraft;

        std::vector<SaveSlotInfo> saveSlots;

        std::string saveNameDraft;
        std::string renameNameDraft;
        std::string statusMessage;

        bool hasActiveGame;
        bool canSaveCurrentGame;
        GameResult lastResult;

        int mainSelected;
        int newGameSelected;
        int settingsSelected;
        int loadSelected;
        int saveSelected;
        int pauseSelected;
        int resultSelected;
        int infoSelected;

        MenuContext()
            : currentScreen(MenuScreen::Main),
            previousScreen(MenuScreen::Main),
            settingsReturnScreen(MenuScreen::Main),
            saveReturnScreen(MenuScreen::Main),
            infoReturnScreen(MenuScreen::Main),
            appSettings(),
            newGameDraft(),
            saveSlots(),
            saveNameDraft(""),
            renameNameDraft(""),
            statusMessage(""),
            hasActiveGame(false),
            canSaveCurrentGame(false),
            lastResult(GameResult::InProgress),
            mainSelected(0),
            newGameSelected(0),
            settingsSelected(0),
            loadSelected(0),
            saveSelected(0),
            pauseSelected(0),
            resultSelected(0),
            infoSelected(0) {
        }
    };

    void InitializeMenuContext(MenuContext& context, const GameSettings& initialSettings);

    void SyncNewGameDraftFromAppSettings(MenuContext& context);

    void SetMenuStatusMessage(MenuContext& context, const std::string& message);
    void ClearMenuStatusMessage(MenuContext& context);

    void SetMenuSaveSlots(MenuContext& context, const std::vector<SaveSlotInfo>& slots);
    void SetMenuHasActiveGame(MenuContext& context, bool value);
    void SetMenuCanSaveCurrentGame(MenuContext& context, bool value);
    void SetMenuLastResult(MenuContext& context, GameResult result);

    void SetMenuSaveNameDraft(MenuContext& context, const std::string& saveName);
    void SetMenuRenameNameDraft(MenuContext& context, const std::string& renameName);
    void SetMenuNewGameNames(MenuContext& context, const std::string& playerX, const std::string& playerO);

    void OpenMainMenuScreen(MenuContext& context);
    void OpenNewGameMenuScreen(MenuContext& context);
    void OpenSettingsMenuScreen(MenuContext& context, MenuScreen fromScreen);
    void OpenLoadMenuScreen(MenuContext& context);
    void OpenSaveMenuScreen(MenuContext& context, const std::string& suggestedSaveName);
    void OpenPauseMenuScreen(MenuContext& context);
    void OpenResultMenuScreen(MenuContext& context, GameResult result);
    void OpenHowToPlayScreen(MenuContext& context, MenuScreen fromScreen);
    void OpenAboutUsScreen(MenuContext& context, MenuScreen fromScreen);

} // namespace caro