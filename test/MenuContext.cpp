#include "MenuContext.h"
#include "MenuText.h"

namespace caro {

    namespace {

        std::string GetDefaultSaveName() {
            return "New Save";
        }

        std::string GetDefaultPlayer1Name(Language language) {
            return (language == Language::Vietnamese) ? "Nguoi choi 1" : "Player 1";
        }

        std::string GetDefaultPlayer2Name(Language language) {
            return (language == Language::Vietnamese) ? "Nguoi choi 2" : "Player 2";
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
        context.newGameDraft.playerXName = GetDefaultPlayer1Name(initialSettings.language);
        context.newGameDraft.playerOName = (initialSettings.gameMode == GameMode::PVE)
            ? GetBotDisplayName(initialSettings.aiDifficulty, initialSettings.language)
            : GetDefaultPlayer2Name(initialSettings.language);

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

        context.newGameDraft.playerXName = GetDefaultPlayer1Name(context.newGameDraft.settings.language);

        if (context.newGameDraft.settings.gameMode == GameMode::PVE) {
            context.newGameDraft.playerOName = GetBotDisplayName(
                context.newGameDraft.settings.aiDifficulty,
                context.newGameDraft.settings.language
            );
        }
        else {
            context.newGameDraft.playerOName = GetDefaultPlayer2Name(context.newGameDraft.settings.language);
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
        context.newGameDraft.playerXName = playerX.empty()
            ? GetDefaultPlayer1Name(context.newGameDraft.settings.language)
            : playerX;

        if (context.newGameDraft.settings.gameMode == GameMode::PVE) {
            context.newGameDraft.playerOName = GetBotDisplayName(
                context.newGameDraft.settings.aiDifficulty,
                context.newGameDraft.settings.language
            );
        }
        else {
            context.newGameDraft.playerOName = playerO.empty()
                ? GetDefaultPlayer2Name(context.newGameDraft.settings.language)
                : playerO;
        }
    }

    void OpenMainMenuScreen(MenuContext& context) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::Main;
        ClearMenuStatusMessage(context);
        context.winnerDisplayName.clear();
    }

    void OpenNewGameMenuScreen(MenuContext& context) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::NewGame;
        SyncNewGameDraftFromAppSettings(context);
        ClearMenuStatusMessage(context);
        context.winnerDisplayName.clear();
    }

    void OpenSettingsMenuScreen(MenuContext& context, MenuScreen fromScreen) {
        context.previousScreen = fromScreen;
        context.settingsReturnScreen = fromScreen;
        context.currentScreen = MenuScreen::Settings;
        ClearMenuStatusMessage(context);
    }

    void OpenLoadMenuScreen(MenuContext& context) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::LoadGame;
        ClearMenuStatusMessage(context);
    }

    void OpenSaveMenuScreen(MenuContext& context, const std::string& suggestedSaveName) {
        context.previousScreen = context.currentScreen;
        context.saveReturnScreen = context.currentScreen;
        context.currentScreen = MenuScreen::SaveGame;
        context.saveNameDraft = suggestedSaveName.empty() ? GetDefaultSaveName() : suggestedSaveName;
        ClearMenuStatusMessage(context);
    }

    void OpenPauseMenuScreen(MenuContext& context) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::Pause;
        ClearMenuStatusMessage(context);
    }

    void OpenResultMenuScreen(MenuContext& context, GameResult result) {
        context.previousScreen = context.currentScreen;
        context.currentScreen = MenuScreen::Result;
        context.lastResult = result;
        ClearMenuStatusMessage(context);
    }

    void OpenHowToPlayScreen(MenuContext& context, MenuScreen fromScreen) {
        context.previousScreen = fromScreen;
        context.infoReturnScreen = fromScreen;
        context.currentScreen = MenuScreen::HowToPlay;
        ClearMenuStatusMessage(context);
    }

    void OpenAboutUsScreen(MenuContext& context, MenuScreen fromScreen) {
        context.previousScreen = fromScreen;
        context.infoReturnScreen = fromScreen;
        context.currentScreen = MenuScreen::AboutUs;
        ClearMenuStatusMessage(context);
    }

} // namespace caro