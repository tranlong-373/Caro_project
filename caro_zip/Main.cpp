#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <array>
#include <conio.h>
#include <direct.h>

#define NOMINMAX
#include <windows.h>

#include "GameAPI.h"
#include "MenuFacade.h"
#include "MenuText.h"
#include "ConsoleAudio.h"
#include "ConsoleRenderer.h"

using namespace std;
using namespace caro;

namespace {

    enum class InGameFlowAction {
        ContinuePlaying = 0,
        RestartMatch = 1,
        BackToMainMenu = 2
    };
    enum class GameplayInputAction {
        None = 0,
        Up,
        Down,
        Left,
        Right,
        Confirm,
        Pause
    };

    struct GameplayInputState {
        std::array<bool, 256> keyDown;

        GameplayInputState() {
            keyDown.fill(false);
        }
    };

    string ChooseText(Language lang, const string& vi, const string& en) {
        return (lang == Language::Vietnamese) ? vi : en;
    }

    string LocalizedGameMode(GameMode mode, Language lang) {
        if (mode == GameMode::PVP) return "PVP";
        return "PVE";
    }

    string LocalizedDifficulty(AIDifficulty difficulty, Language lang) {
        switch (difficulty) {
        case AIDifficulty::Easy:   return ChooseText(lang, "De", "Easy");
        case AIDifficulty::Medium: return ChooseText(lang, "Trung binh", "Medium");
        case AIDifficulty::Hard:   return ChooseText(lang, "Kho", "Hard");
        case AIDifficulty::Master: return ChooseText(lang, "Cao thu", "Master");
        default:                   return "Unknown";
        }
    }

    string GetDefaultPlayer1Name(Language lang) {
        return ChooseText(lang, "Nguoi choi 1", "Player 1");
    }

    string GetDefaultPlayer2Name(Language lang) {
        return ChooseText(lang, "Nguoi choi 2", "Player 2");
    }

    string GetBotName(AIDifficulty difficulty, Language lang) {
        return "BOT - " + LocalizedDifficulty(difficulty, lang);
    }

    string NormalizeSaveDirectory() {
        string dir = config::DEFAULT_SAVE_DIRECTORY;
        while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) {
            dir.pop_back();
        }
        if (dir.empty()) dir = "saves";
        return dir;
    }

    void EnsureSaveDirectory() {
        string dir = NormalizeSaveDirectory();
        _mkdir(dir.c_str());
    }

    string BuildSavePath(int slot) {
        ostringstream oss;
        oss << NormalizeSaveDirectory()
            << "/slot"
            << slot
            << config::DEFAULT_SAVE_EXTENSION;
        return oss.str();
    }

    bool FileExists(const string& path) {
        ifstream fin(path.c_str());
        return fin.good();
    }

    int ReadKeyUpper() {
        int ch = _getch();

        if (ch == 0 || ch == 224) {
            int ext = _getch();

            if (ext == 72) return 'W';
            if (ext == 80) return 'S';
            if (ext == 75) return 'A';
            if (ext == 77) return 'D';

            return -1;
        }

        if (ch == 13) return 13;
        if (ch == 27) return 27;

        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
        return ch;
    }

    bool IsVirtualKeyDown(int vk) {
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    void ClearPendingConsoleInputKeys() {
        HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
        FlushConsoleInputBuffer(in);

        while (_kbhit()) {
            _getch();
        }
    }

    void WaitUntilGameplayKeysReleased() {
        while (
            IsVirtualKeyDown('W') || IsVirtualKeyDown('A') || IsVirtualKeyDown('S') || IsVirtualKeyDown('D') ||
            IsVirtualKeyDown('I') || IsVirtualKeyDown('J') || IsVirtualKeyDown('K') || IsVirtualKeyDown('L') ||
            IsVirtualKeyDown(VK_UP) || IsVirtualKeyDown(VK_DOWN) || IsVirtualKeyDown(VK_LEFT) || IsVirtualKeyDown(VK_RIGHT) ||
            IsVirtualKeyDown('P') || IsVirtualKeyDown(VK_RETURN) || IsVirtualKeyDown(VK_ESCAPE)
            ) {
            Sleep(10);
        }
    }

    void ResetGameplayInputState(GameplayInputState& state) {
        state = GameplayInputState();
    }

    GameplayInputAction MapGameplayVirtualKey(WORD vk) {
        switch (vk) {
        case 'W':
        case 'I':
        case VK_UP:
            return GameplayInputAction::Up;

        case 'S':
        case 'K':
        case VK_DOWN:
            return GameplayInputAction::Down;

        case 'A':
        case 'J':
        case VK_LEFT:
            return GameplayInputAction::Left;

        case 'D':
        case 'L':
        case VK_RIGHT:
            return GameplayInputAction::Right;

        case VK_RETURN:
            return GameplayInputAction::Confirm;

        case 'P':
        case VK_ESCAPE:
            return GameplayInputAction::Pause;

        default:
            return GameplayInputAction::None;
        }
    }

    void FlushGameplayInputBuffer() {
        HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
        FlushConsoleInputBuffer(in);
    }

    GameplayInputAction ReadGameplayInput(GameplayInputState& state) {
        HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
        DWORD eventCount = 0;

        if (!GetNumberOfConsoleInputEvents(in, &eventCount) || eventCount == 0) {
            return GameplayInputAction::None;
        }

        while (eventCount > 0) {
            INPUT_RECORD record;
            DWORD readCount = 0;

            if (!ReadConsoleInput(in, &record, 1, &readCount) || readCount == 0) {
                return GameplayInputAction::None;
            }

            if (record.EventType == KEY_EVENT) {
                KEY_EVENT_RECORD key = record.Event.KeyEvent;
                WORD vk = key.wVirtualKeyCode;
                bool trackedKey = vk < state.keyDown.size();

                if (!key.bKeyDown) {
                    if (trackedKey) {
                        state.keyDown[vk] = false;
                    }
                }
                else {
                    if (trackedKey && state.keyDown[vk]) {
                        GetNumberOfConsoleInputEvents(in, &eventCount);
                        continue;
                    }

                    if (trackedKey) {
                        state.keyDown[vk] = true;
                    }

                    GameplayInputAction action = MapGameplayVirtualKey(vk);
                    if (action != GameplayInputAction::None) {
                        return action;
                    }
                }
            }

            GetNumberOfConsoleInputEvents(in, &eventCount);
        }

        return GameplayInputAction::None;
    }

    void ClearScreen() {
        ClearScreenFast();
    }

    string PromptLine(const string& title, const string& prompt, const string& defaultValue) {
        ClearScreen();
        ClearPendingConsoleInputKeys();

        cout << title << "\n\n";
        cout << prompt;

        if (!defaultValue.empty()) {
            cout << " [" << defaultValue << "]";
        }

        cout << "\n> ";

        string line;
        getline(cin, line);

        ClearPendingConsoleInputKeys();

        if (line.empty()) return defaultValue;
        return line;
    }

    bool PromptYesNo(const string& title, const string& question, Language lang) {
        ClearScreen();
        ClearPendingConsoleInputKeys();

        cout << title << "\n\n";
        cout << question << "\n";
        cout << ChooseText(lang, "Nhan Y de dong y, phim bat ky de huy.\n", "Press Y to accept, any other key to cancel.\n");

        int key = ReadKeyUpper();

        ClearPendingConsoleInputKeys();
        return key == 'Y';
    }

    string GetWinnerDisplayName(const GameSession& game) {
        if (game.result == GameResult::XWin) {
            return game.playerX.name;
        }

        if (game.result == GameResult::OWin) {
            if (game.settings.gameMode == GameMode::PVE) {
                return "BOT";
            }
            return game.playerO.name;
        }

        return "";
    }

    bool IsBotTurn(const GameSession& game) {
        return game.settings.gameMode == GameMode::PVE
            && game.currentTurn == CellState::O
            && game.result == GameResult::InProgress;
    }

    Position FindFirstEmptyCell(const GameSession& game) {
        int n = GetBoardSize(game);
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                Position p(r, c);
                if (IsCellEmpty(game, p)) return p;
            }
        }
        return Position(0, 0);
    }

    Position FindDefaultCursor(const GameSession& game) {
        int n = GetBoardSize(game);
        Position center(n / 2, n / 2);

        if (IsInsideBoard(game, center) && IsCellEmpty(game, center)) {
            return center;
        }

        return FindFirstEmptyCell(game);
    }

    void DrawGameScreen(const GameSession& game, const Position& cursor, const string& message) {
        DrawGameScreenEnhanced(game, cursor, message);
    }

    void DrawGameResultScreen(
        const GameSession& game,
        const Position& cursor,
        const string& message,
        const MenuContext& menu,
        const vector<Position>& winningCells
    ) {
        MenuView resultView = BuildMenuView(menu);
        DrawGameResultScreenEnhanced(game, cursor, message, resultView, &winningCells, true);
    }

    string BuildAutoSaveName(const GameSession& game, int slot) {
        const Language lang = game.settings.language;

        ostringstream oss;
        oss << ChooseText(lang, "slot", "slot")
            << slot
            << "_"
            << LocalizedGameMode(game.settings.gameMode, lang)
            << "_"
            << game.moveCount
            << ChooseText(lang, "nuoc", "moves");
        return oss.str();
    }

    string BuildSuggestedSaveName(const GameSession& game) {
        if (!game.currentSaveName.empty()) return game.currentSaveName;

        const Language lang = game.settings.language;

        ostringstream oss;
        oss << LocalizedGameMode(game.settings.gameMode, lang)
            << "_"
            << game.playerX.name
            << "_vs_"
            << game.playerO.name
            << "_"
            << game.moveCount
            << ChooseText(lang, "nuoc", "moves");
        return oss.str();
    }

    MenuInput ReadMenuInputFromKeyboard() {
        int key = ReadKeyUpper();

        if (key == 'W' || key == 'I') return MenuInput::Up;
        if (key == 'S' || key == 'K') return MenuInput::Down;
        if (key == 'A' || key == 'J') return MenuInput::Left;
        if (key == 'D' || key == 'L') return MenuInput::Right;
        if (key == 13) return MenuInput::Confirm;
        if (key == 27) return MenuInput::Back;

        return MenuInput::None;
    }

    void DrawMenuView(const MenuView& view) {
        DrawMenuViewEnhanced(view);
    }

    void PlayMenuFeedback(const GameSettings& settings, MenuInput input) {
        if (input == MenuInput::Up || input == MenuInput::Down ||
            input == MenuInput::Left || input == MenuInput::Right) {
            PlayMenuMoveSound(settings);
        }
        else if (input == MenuInput::Confirm || input == MenuInput::Back) {
            PlayConfirmSound(settings);
        }
    }

    void ApplyMenuSettingsToGame(MenuContext& menu, GameSession& game) {
        game.settings.soundEnabled = menu.appSettings.soundEnabled;
        game.settings.musicEnabled = menu.appSettings.musicEnabled;
        game.settings.soundVolume = menu.appSettings.soundVolume;
        game.settings.musicVolume = menu.appSettings.musicVolume;
        game.settings.language = menu.appSettings.language;

        if (game.settings.gameMode == GameMode::PVE) {
            game.playerO.name = GetBotName(game.settings.aiDifficulty, game.settings.language);
        }
    }

    void SyncMenuSettingsFromGame(MenuContext& menu, const GameSession& game) {
        menu.appSettings = game.settings;
        SyncNewGameDraftFromAppSettings(menu);
        UpdateMenuSaveNameDraft(menu, BuildSuggestedSaveName(game));
    }

    void RefreshMenuContextForCurrentGame(MenuContext& menu, const GameSession& game) {
        RefreshMenuSaveSlotsFromFiles(menu);
        UpdateMenuGameFlags(menu, true, true);
        UpdateMenuLastResult(menu, game.result);
        UpdateMenuSaveNameDraft(menu, BuildSuggestedSaveName(game));
    }

    bool SaveGameToSlot(GameSession& game, int slotIndex, const string& saveName, string& message) {
        string path = BuildSavePath(slotIndex);

        game.currentSaveName = saveName.empty()
            ? BuildAutoSaveName(game, slotIndex)
            : saveName;

        if (SaveGameToFile(game, path)) {
            message = ChooseText(
                game.settings.language,
                "Luu thanh cong vao o " + to_string(slotIndex),
                "Save successful to slot " + to_string(slotIndex)
            );
            return true;
        }

        message = ChooseText(game.settings.language, "Luu that bai", "Save failed");
        return false;
    }

    bool LoadGameFromSlot(GameSession& game, int slotIndex, string& message) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = ChooseText(game.settings.language, "O nay dang trong", "This slot is empty");
            return false;
        }

        if (!LoadGameFromFile(game, path)) {
            message = ChooseText(game.settings.language, "Tai that bai", "Load failed");
            return false;
        }

        game.screen = ScreenState::Playing;
        game.isPaused = false;

        if (game.settings.gameMode == GameMode::PVE) {
            game.playerO.name = GetBotName(game.settings.aiDifficulty, game.settings.language);
        }

        message = ChooseText(game.settings.language, "Tai thanh cong", "Load successful");
        return true;
    }

    bool RenameSaveSlotMetadata(int slotIndex, const string& newName, string& message, Language lang) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = ChooseText(lang, "O nay dang trong", "This slot is empty");
            return false;
        }

        GameSession temp;
        if (!LoadGameFromFile(temp, path)) {
            message = ChooseText(lang, "Khong doc duoc file luu de doi ten", "Cannot read save to rename");
            return false;
        }

        temp.currentSaveName = newName.empty() ? ("slot" + to_string(slotIndex)) : newName;

        if (!SaveGameToFile(temp, path)) {
            message = ChooseText(lang, "Doi ten that bai", "Rename save failed");
            return false;
        }

        message = ChooseText(
            lang,
            "Da doi ten o luu " + to_string(slotIndex),
            "Renamed save in slot " + to_string(slotIndex)
        );
        return true;
    }

    bool DeleteSaveSlotFile(int slotIndex, string& message, Language lang) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = ChooseText(lang, "O nay dang trong", "This slot is empty");
            return false;
        }

        if (!DeleteSaveFile(path)) {
            message = ChooseText(lang, "Xoa file luu that bai", "Delete save failed");
            return false;
        }

        message = ChooseText(
            lang,
            "Da xoa file luu o " + to_string(slotIndex),
            "Deleted save in slot " + to_string(slotIndex)
        );
        return true;
    }

    void AskPlayerNamesForNewGame(MenuCommand& command) {
        const Language lang = command.settings.language;

        string defaultX = command.text.empty() ? GetDefaultPlayer1Name(lang) : command.text;
        string defaultO = command.extraText.empty() ? GetDefaultPlayer2Name(lang) : command.extraText;

        string playerX = PromptLine(
            ChooseText(lang, "CHOI MOI", "NEW GAME"),
            ChooseText(lang, "Nhap ten Nguoi choi 1", "Enter Player 1 name"),
            defaultX
        );
        command.text = playerX;

        if (command.settings.gameMode == GameMode::PVE) {
            command.extraText = GetBotName(command.settings.aiDifficulty, lang);
        }
        else {
            string playerO = PromptLine(
                ChooseText(lang, "CHOI MOI", "NEW GAME"),
                ChooseText(lang, "Nhap ten Nguoi choi 2", "Enter Player 2 name"),
                defaultO
            );
            command.extraText = playerO;
        }
    }

    void HandleSaveCommand(MenuContext& menu, GameSession& game, MenuCommand& command) {
        const Language lang = game.settings.language;

        string defaultSaveName = command.text.empty()
            ? BuildSuggestedSaveName(game)
            : command.text;

        string saveName = PromptLine(
            ChooseText(lang, "LUU GAME", "SAVE GAME"),
            ChooseText(lang, "Nhap ten ban luu", "Enter save name"),
            defaultSaveName
        );

        string message;
        SaveGameToSlot(game, command.slotIndex, saveName, message);

        UpdateMenuSaveNameDraft(menu, saveName);
        SetMenuStatusMessage(menu, message);
        RefreshMenuSaveSlotsFromFiles(menu);
    }

    void HandleRenameCommand(MenuContext& menu, MenuCommand& command) {
        const Language lang = menu.appSettings.language;

        string currentName = command.text;
        if (currentName.empty()) {
            currentName = "slot" + to_string(command.slotIndex);
        }

        string newName = PromptLine(
            ChooseText(lang, "DOI TEN SAVE", "RENAME SAVE"),
            ChooseText(lang, "Nhap ten moi", "Enter new save name"),
            currentName
        );

        string message;
        RenameSaveSlotMetadata(command.slotIndex, newName, message, lang);

        UpdateMenuRenameDraft(menu, newName);
        SetMenuStatusMessage(menu, message);
        RefreshMenuSaveSlotsFromFiles(menu);
    }

    void HandleDeleteCommand(MenuContext& menu, MenuCommand& command) {
        const Language lang = menu.appSettings.language;

        bool accepted = PromptYesNo(
            ChooseText(lang, "XOA SAVE", "DELETE SAVE"),
            ChooseText(
                lang,
                "Ban co chac muon xoa o luu " + to_string(command.slotIndex) + "?",
                "Are you sure you want to delete slot " + to_string(command.slotIndex) + "?"
            ),
            lang
        );

        if (!accepted) {
            SetMenuStatusMessage(menu, ChooseText(lang, "Da huy xoa", "Delete canceled"));
            return;
        }

        string message;
        DeleteSaveSlotFile(command.slotIndex, message, lang);
        SetMenuStatusMessage(menu, message);
        RefreshMenuSaveSlotsFromFiles(menu);
    }

    void ShowResultTransition(const GameSession& game, const Position& cursor, const string& message) {
        vector<Position> winningCells = FindWinningLineCells(game);
        PlayResultSound(game.settings, game.result);

        if (winningCells.empty()) {
            DrawGameScreenEnhanced(game, cursor, message);
            SleepMs(350);
            return;
        }

        for (int i = 0; i < 10; ++i) {
            if (i % 2 == 0) {
                DrawGameScreenEnhanced(game, cursor, message, &winningCells, true);
            }
            else {
                DrawGameScreenEnhanced(game, cursor, message, 0, false);
            }
            SleepMs(120);
        }

        DrawGameScreenEnhanced(game, cursor, message, &winningCells, true);
    }

    InGameFlowAction RunInGameMenuLoop(
        MenuContext& menu,
        GameSession& game,
        const Position* resultCursor = 0,
        const string* resultMessage = 0,
        const vector<Position>* winningCells = 0
    ) {
        while (true) {
            RefreshMenuContextForCurrentGame(menu, game);

            MenuView view = BuildMenuView(menu);
            if (menu.currentScreen == MenuScreen::Result &&
                resultCursor != 0 && resultMessage != 0 && winningCells != 0) {
                DrawGameResultScreenEnhanced(game, *resultCursor, *resultMessage, view, winningCells, true);
            }
            else {
                DrawMenuView(view);
            }

            MenuInput input = ReadMenuInputFromKeyboard();
            PlayMenuFeedback(menu.appSettings, input);
            MenuCommand command = HandleMenuInput(menu, input);

            switch (command.type) {
            case MenuCommandType::None:
                break;

            case MenuCommandType::ApplySettings:
                ApplyMenuSettingsToGame(menu, game);
                SetMenuStatusMessage(menu, ChooseText(menu.appSettings.language, "Da cap nhat cai dat", "Settings updated"));
                break;

            case MenuCommandType::OpenSaveMenu:
            case MenuCommandType::OpenSettings:
            case MenuCommandType::OpenHowToPlay:
            case MenuCommandType::OpenAboutUs:
                break;

            case MenuCommandType::RequestSaveSlot:
                HandleSaveCommand(menu, game, command);
                break;

            case MenuCommandType::ContinueGame:
                return InGameFlowAction::ContinuePlaying;

            case MenuCommandType::RestartGame:
                return InGameFlowAction::RestartMatch;

            case MenuCommandType::BackToMainMenu:
                return InGameFlowAction::BackToMainMenu;

            default:
                break;
            }
        }
    }

    void RunBotMove(GameSession& game, string& message) {
        if (!IsBotTurn(game)) return;

        Position aiMove = FindBestAIMove(game);
        if (!IsValidPosition(aiMove)) return;

        ActionResult result = PlaceCurrentTurn(game, aiMove);

        if (result == ActionResult::Success) {
            PlayPlaceSound(game.settings, CellState::O);

            ostringstream oss;
            if (game.settings.language == Language::Vietnamese) {
                oss << "BOT danh tai (" << aiMove.row << ", " << aiMove.col << ")";
            }
            else {
                oss << "BOT moved to (" << aiMove.row << ", " << aiMove.col << ")";
            }
            message = oss.str();
        }
    }

    void RunGameLoop(GameSession& game, MenuContext& menu) {
        Position cursor = FindDefaultCursor(game);
        string message;

        bool needsRedraw = true;
        bool resultMenuOpenedForThisMatch = false;

        GameplayInputState inputState;

        SyncMenuSettingsFromGame(menu, game);

        const DWORD idleSleepMs = 1;

        while (true) {
            if (IsBotTurn(game)) {
                RunBotMove(game, message);
                needsRedraw = true;
            }

            if (game.result != GameResult::InProgress) {
                if (!resultMenuOpenedForThisMatch) {
                    if (needsRedraw) {
                        DrawGameScreen(game, cursor, message);
                        needsRedraw = false;
                    }

                    ShowResultTransition(game, cursor, message);

                    ResetGameplayInputState(inputState);
                    WaitUntilGameplayKeysReleased();
                    FlushGameplayInputBuffer();
                    ClearPendingConsoleInputKeys();

                    SetMenuWinnerDisplayName(menu, GetWinnerDisplayName(game));
                    OpenResultMenu(menu, game.result);

                    resultMenuOpenedForThisMatch = true;
                }

                vector<Position> winningCells = FindWinningLineCells(game);
                InGameFlowAction action = RunInGameMenuLoop(menu, game, &cursor, &message, &winningCells);

                ResetGameplayInputState(inputState);
                WaitUntilGameplayKeysReleased();
                FlushGameplayInputBuffer();
                ClearPendingConsoleInputKeys();

                if (action == InGameFlowAction::RestartMatch) {
                    ResetCurrentMatch(game);
                    ApplyMenuSettingsToGame(menu, game);
                    cursor = FindDefaultCursor(game);
                    message = ChooseText(game.settings.language, "Tran moi bat dau lai", "Match restarted");
                    needsRedraw = true;
                    resultMenuOpenedForThisMatch = false;
                    continue;
                }

                if (action == InGameFlowAction::BackToMainMenu) {
                    return;
                }

                needsRedraw = true;
                continue;
            }

            resultMenuOpenedForThisMatch = false;

            if (needsRedraw) {
                DrawGameScreen(game, cursor, message);
                needsRedraw = false;
            }

            GameplayInputAction input = ReadGameplayInput(inputState);
            bool moved = false;
            bool pausePressed = false;

            switch (input) {
            case GameplayInputAction::Up:
                if (cursor.row > 0) {
                    cursor.row--;
                    moved = true;
                }
                break;

            case GameplayInputAction::Down:
                if (cursor.row + 1 < GetBoardSize(game)) {
                    cursor.row++;
                    moved = true;
                }
                break;

            case GameplayInputAction::Left:
                if (cursor.col > 0) {
                    cursor.col--;
                    moved = true;
                }
                break;

            case GameplayInputAction::Right:
                if (cursor.col + 1 < GetBoardSize(game)) {
                    cursor.col++;
                    moved = true;
                }
                break;

            case GameplayInputAction::Confirm: {
                CellState placingSymbol = game.currentTurn;
                ActionResult result = PlaceCurrentTurn(game, cursor);

                if (result == ActionResult::Success) {
                    PlayPlaceSound(game.settings, placingSymbol);
                    message = ChooseText(game.settings.language, "Dat quan thanh cong", "Placed successfully");
                    needsRedraw = true;

                    ResetGameplayInputState(inputState);
                    FlushGameplayInputBuffer();
                    ClearPendingConsoleInputKeys();
                }
                else {
                    PlayInvalidSound(game.settings);
                }
                break;
            }

            case GameplayInputAction::Pause:
                pausePressed = true;
                PlayConfirmSound(game.settings);
                break;

            case GameplayInputAction::None:
            default:
                break;
            }

            if (moved) {
                needsRedraw = true;
            }

            if (pausePressed) {
                ResetGameplayInputState(inputState);
                WaitUntilGameplayKeysReleased();
                FlushGameplayInputBuffer();
                ClearPendingConsoleInputKeys();

                OpenPauseMenu(menu);

                InGameFlowAction action = RunInGameMenuLoop(menu, game);

                ResetGameplayInputState(inputState);
                WaitUntilGameplayKeysReleased();
                FlushGameplayInputBuffer();
                ClearPendingConsoleInputKeys();

                if (action == InGameFlowAction::ContinuePlaying) {
                    message = ChooseText(game.settings.language, "Tiep tuc tran dau", "Continue game");
                    needsRedraw = true;
                }
                else if (action == InGameFlowAction::RestartMatch) {
                    ResetCurrentMatch(game);
                    ApplyMenuSettingsToGame(menu, game);
                    cursor = FindDefaultCursor(game);
                    message = ChooseText(game.settings.language, "Tran moi bat dau lai", "Match restarted");
                    needsRedraw = true;
                    resultMenuOpenedForThisMatch = false;
                }
                else if (action == InGameFlowAction::BackToMainMenu) {
                    return;
                }
            }

            Sleep(idleSleepMs);
        }
    }

} // namespace

int main() {
    InitializeConsoleUI();
    EnsureSaveDirectory();

    GameSession game;
    MenuContext menu;

    InitializeMenuSystem(menu, CreateDefaultSettings());
    RefreshMenuSaveSlotsFromFiles(menu);
    OpenMainMenuScreen(menu);

    bool running = true;

    while (running) {
        RefreshMenuSaveSlotsFromFiles(menu);
        UpdateMenuGameFlags(menu, false, false);

        MenuView view = BuildMenuView(menu);
        DrawMenuView(view);

        MenuInput input = ReadMenuInputFromKeyboard();
        PlayMenuFeedback(menu.appSettings, input);
        MenuCommand command = HandleMenuInput(menu, input);

        switch (command.type) {
        case MenuCommandType::None:
            break;

        case MenuCommandType::OpenLoadMenu:
        case MenuCommandType::OpenSettings:
        case MenuCommandType::OpenHowToPlay:
        case MenuCommandType::OpenAboutUs:
            break;

        case MenuCommandType::ApplySettings:
            SetMenuStatusMessage(menu, ChooseText(menu.appSettings.language, "Da cap nhat cai dat", "Settings updated"));
            break;

        case MenuCommandType::RequestRenameSlot:
            HandleRenameCommand(menu, command);
            break;

        case MenuCommandType::RequestDeleteSlot:
            HandleDeleteCommand(menu, command);
            break;

        case MenuCommandType::RequestLoadSlot: {
            string message;
            if (LoadGameFromSlot(game, command.slotIndex, message)) {
                SyncMenuSettingsFromGame(menu, game);
                SetMenuStatusMessage(menu, message);
                RunGameLoop(game, menu);
                OpenMainMenuScreen(menu);
            }
            else {
                SetMenuStatusMessage(menu, message);
            }
            break;
        }

        case MenuCommandType::StartNewGame: {
            AskPlayerNamesForNewGame(command);

            StartNewGame(game, command.settings, command.text, command.extraText);
            PlayStartGameSound(command.settings);
            ApplyMenuSettingsToGame(menu, game);
            SyncMenuSettingsFromGame(menu, game);

            RunGameLoop(game, menu);
            OpenMainMenuScreen(menu);
            break;
        }

        case MenuCommandType::ExitApplication:
            running = false;
            break;

        default:
            break;
        }
    }

    ClearScreen();
    cout << "Goodbye!\n";
    return 0;
}