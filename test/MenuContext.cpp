#include "MenuContext.h"
#include "MenuText.h"

namespace caro {

    namespace {

        std::string GetDefaultSaveName() {
            return "New Save";
        }

        std::string GetDefaultPlayer1Name() {
            return "Player 1";
        }

        std::string GetDefaultPlayer2Name() {
            return "Player 2";
        }

        std::string GetBotName(AIDifficulty difficulty) {
            return "BOT - " + GetDisplayText(difficulty);
        }

    } // namespace

    void InitializeMenuContext(MenuContext& context, const GameSettings& initialSettings) {
        context.currentScreen = MenuScreen::Main;
        context.previousScreen = MenuScreen::Main;

        context.settingsReturnScreen = MenuScreen::Main;
        context.saveReturnScreen = MenuScreen::Main;
        context.infoReturnScreen = MenuScreen::Main;

        context.appSettings = initialSettings;
        context.newGameDraft.settings = initialSettings;
        context.newGameDraft.playerXName = GetDefaultPlayer1Name();
        context.newGameDraft.playerOName = (initialSettings.gameMode == GameMode::PVE)
            ? GetBotName(initialSettings.aiDifficulty)
            : GetDefaultPlayer2Name();

        context.saveSlots.clear();
        context.saveNameDraft = GetDefaultSaveName();
        context.renameNameDraft.clear();
        context.statusMessage.clear();
        context.winnerDisplayName.clear();

        context.hasActiveGame = false;
        context.canSaveCurrentGame = false;
        context.lastResult = GameResult::InProgress;

        context.mainSelected = 0;
        context.newGameSelected = 0;
        context.settingsSelected = 0;
        context.loadSelected = 0;
        context.saveSelected = 0;
        context.pauseSelected = 0;
        context.resultSelected = 0;
        context.infoSelected = 0;
    }

    void SyncNewGameDraftFromAppSettings(MenuContext& context) {
        context.newGameDraft.settings = context.appSettings;
        context.newGameDraft.playerXName = GetDefaultPlayer1Name();

        if (context.newGameDraft.settings.gameMode == GameMode::PVE) {
            context.newGameDraft.playerOName = GetBotName(context.newGameDraft.settings.aiDifficulty);
        }
        else {
            context.newGameDraft.playerOName = GetDefaultPlayer2Name();
        }
    }

    void SetMenuStatusMessage(MenuContext& context, const std::string& message) {
        context.statusMessage = message;
    }

    void ClearMenuStatusMessage(MenuContext& context) {
        context.statusMessage.clear();
    }

    void SetMenuSaveSlots(MenuContext& context, const std::vector<SaveSlotInfo>& slots) {
        context.saveSlots = slots;
    }

    void SetMenuHasActiveGame(MenuContext& context, bool value) {
        context.hasActiveGame = value;
    }

    void SetMenuCanSaveCurrentGame(MenuContext& context, bool value) {
        context.canSaveCurrentGame = value;
    }

    void SetMenuLastResult(MenuContext& context, GameResult result) {
        context.lastResult = result;
    }

    void SetMenuWinnerDisplayName(MenuContext& context, const std::string& winnerName) {
        context.winnerDisplayName = winnerName;
    }

    void SetMenuSaveNameDraft(MenuContext& context, const std::string& saveName) {
        context.saveNameDraft = saveName;
    }

    void SetMenuRenameNameDraft(MenuContext& context, const std::string& renameName) {
        context.renameNameDraft = renameName;
    }

    void SetMenuNewGameNames(MenuContext& context, const std::string& playerX, const std::string& playerO) {
        context.newGameDraft.playerXName = playerX.empty() ? GetDefaultPlayer1Name() : playerX;

        if (context.newGameDraft.settings.gameMode == GameMode::PVE) {
            context.newGameDraft.playerOName = GetBotName(context.newGameDraft.settings.aiDifficulty);
        }
        else {
            context.newGameDraft.playerOName = playerO.empty() ? GetDefaultPlayer2Name() : playerO;
        }
    }

    void OpenMainMenuScreen(MenuContext& context) {
        context.currentScreen = MenuScreen::Main;
        context.previousScreen = MenuScreen::Main;

        context.settingsReturnScreen = MenuScreen::Main;
        context.saveReturnScreen = MenuScreen::Main;
        context.infoReturnScreen = MenuScreen::Main;

        context.settingsSelected = 0;
        context.saveSelected = 0;
        context.pauseSelected = 0;
        context.resultSelected = 0;
        context.infoSelected = 0;

        context.winnerDisplayName.clear();
        ClearMenuStatusMessage(context);
    }

    void OpenNewGameMenuScreen(MenuContext& context) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::NewGame;
        context.newGameSelected = 0;
        SyncNewGameDraftFromAppSettings(context);
        ClearMenuStatusMessage(context);
        context.winnerDisplayName.clear();
    }

    void OpenSettingsMenuScreen(MenuContext& context, MenuScreen fromScreen) {
        context.previousScreen = fromScreen;
        context.settingsReturnScreen = fromScreen;
        context.currentScreen = MenuScreen::Settings;
        context.settingsSelected = 0;
        ClearMenuStatusMessage(context);
    }

    void OpenLoadMenuScreen(MenuContext& context) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::LoadGame;
        context.loadSelected = 0;
        ClearMenuStatusMessage(context);
    }

    void OpenSaveMenuScreen(MenuContext& context, const std::string& suggestedSaveName) {
        context.previousScreen = context.currentScreen;
        context.saveReturnScreen = context.currentScreen;
        context.currentScreen = MenuScreen::SaveGame;
        context.saveSelected = 0;
        context.saveNameDraft = suggestedSaveName.empty() ? GetDefaultSaveName() : suggestedSaveName;
        ClearMenuStatusMessage(context);
    }

    void OpenPauseMenuScreen(MenuContext& context) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::Pause;
        context.pauseSelected = 0;
        ClearMenuStatusMessage(context);
    }

    void OpenResultMenuScreen(MenuContext& context, GameResult result) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::Result;
        context.resultSelected = 0;
        context.lastResult = result;
        ClearMenuStatusMessage(context);
    }

    void OpenHowToPlayScreen(MenuContext& context, MenuScreen fromScreen) {
        context.previousScreen = fromScreen;
        context.infoReturnScreen = fromScreen;
        context.currentScreen = MenuScreen::HowToPlay;
        context.infoSelected = 0;
        ClearMenuStatusMessage(context);
    }

    void OpenAboutUsScreen(MenuContext& context, MenuScreen fromScreen) {
        context.previousScreen = fromScreen;
        context.infoReturnScreen = fromScreen;
        context.currentScreen = MenuScreen::AboutUs;
        context.infoSelected = 0;
        ClearMenuStatusMessage(context);
    }

} // namespace caro