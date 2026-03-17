#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <conio.h>
#include <direct.h>

#include "GameAPI.h"
#include "MenuFacade.h"
#include "MenuText.h"

using namespace std;
using namespace caro;

namespace {

    enum class InGameFlowAction {
        ContinuePlaying = 0,
        RestartMatch = 1,
        BackToMainMenu = 2
    };

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

        if (ch == 13) return 13;
        if (ch == 27) return 27;

        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
        return ch;
    }

    void ClearScreen() {
        system("cls");
    }

    string PromptLine(const string& title, const string& prompt, const string& defaultValue) {
        ClearScreen();
        cout << title << "\n\n";
        cout << prompt;
        if (!defaultValue.empty()) {
            cout << " [" << defaultValue << "]";
        }
        cout << "\n> ";

        string line;
        getline(cin, line);

        if (line.empty()) return defaultValue;
        return line;
    }

    bool PromptYesNo(const string& title, const string& question) {
        ClearScreen();
        cout << title << "\n\n";
        cout << question << "\n";
        cout << "Nhan Y de dong y, phim bat ky de huy.\n";

        int key = ReadKeyUpper();
        return key == 'Y';
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

    char CellToChar(CellState cell) {
        if (cell == CellState::X) return 'X';
        if (cell == CellState::O) return 'O';
        return '.';
    }

    string ResultToText(GameResult result) {
        switch (result) {
        case GameResult::XWin: return "X wins";
        case GameResult::OWin: return "O wins";
        case GameResult::Draw: return "Draw";
        default: return "In Progress";
        }
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

    void PrintBoardWithCursor(const GameSession& game, const Position& cursor) {
        int n = GetBoardSize(game);

        cout << "     ";
        for (int c = 0; c < n; ++c) {
            cout << setw(3) << c;
        }
        cout << "\n";

        for (int r = 0; r < n; ++r) {
            cout << setw(3) << r << "  ";
            for (int c = 0; c < n; ++c) {
                char ch = CellToChar(GetCell(game, Position(r, c)));

                if (r == cursor.row && c == cursor.col) {
                    cout << "[" << ch << "]";
                }
                else {
                    cout << " " << ch << " ";
                }
            }
            cout << "\n";
        }
    }

    void PrintGameHeader(const GameSession& game, const string& message) {
        cout << "=========== CARO CONSOLE ===========\n\n";
        cout << "Mode      : " << ToString(game.settings.gameMode) << "\n";
        cout << "Rule      : " << ToString(game.settings.ruleMode) << "\n";
        cout << "Player X  : " << game.playerX.name << "\n";
        cout << "Player O  : " << game.playerO.name << "\n";
        cout << "Turn      : " << ToString(game.currentTurn) << "\n";
        cout << "Result    : " << ResultToText(game.result) << "\n";
        cout << "Moves     : " << game.moveCount << "\n";
        cout << "Sound     : " << (game.settings.soundEnabled ? "ON" : "OFF")
            << " (" << game.settings.soundVolume << "%)\n";
        cout << "Music     : " << (game.settings.musicEnabled ? "ON" : "OFF")
            << " (" << game.settings.musicVolume << "%)\n";
        cout << "Language  : " << ToString(game.settings.language) << "\n";

        if (!game.currentSaveName.empty()) {
            cout << "Save name : " << game.currentSaveName << "\n";
        }

        if (!message.empty()) {
            cout << "Message   : " << message << "\n";
        }

        cout << "\n";
    }

    void PrintControls() {
        cout << "\n===== CONTROLS =====\n";
        cout << "W/A/S/D : Move\n";
        cout << "Enter   : Place piece / confirm\n";
        cout << "P / ESC : Pause menu\n";
        cout << "In menu : W/S move, A/D change, Enter confirm\n";
    }

    void DrawGameScreen(const GameSession& game, const Position& cursor, const string& message) {
        ClearScreen();
        PrintGameHeader(game, message);
        PrintBoardWithCursor(game, cursor);
        PrintControls();
    }

    string BuildAutoSaveName(const GameSession& game, int slot) {
        ostringstream oss;
        oss << "slot" << slot
            << "_"
            << ToString(game.settings.gameMode)
            << "_"
            << game.moveCount
            << "moves";
        return oss.str();
    }

    string BuildSuggestedSaveName(const GameSession& game) {
        if (!game.currentSaveName.empty()) return game.currentSaveName;

        ostringstream oss;
        oss << ToString(game.settings.gameMode)
            << "_"
            << game.playerX.name
            << "_vs_"
            << game.playerO.name
            << "_"
            << game.moveCount
            << "moves";
        return oss.str();
    }

    MenuInput ReadMenuInputFromKeyboard() {
        int key = ReadKeyUpper();

        if (key == 'W') return MenuInput::Up;
        if (key == 'S') return MenuInput::Down;
        if (key == 'A') return MenuInput::Left;
        if (key == 'D') return MenuInput::Right;
        if (key == 13)  return MenuInput::Confirm;
        if (key == 27)  return MenuInput::Back;

        return MenuInput::None;
    }

    void PrintSingleMenuItem(const MenuItemView& item) {
        string prefix = item.selected ? " > " : "   ";
        string disabled = item.enabled ? "" : "[Locked] ";

        cout << prefix << disabled << item.label;

        if (!item.value.empty() && item.kind != MenuItemKind::SaveSlot) {
            cout << " : " << item.value;
        }

        cout << "\n";

        if (item.selected && !item.hint.empty()) {
            cout << "     " << item.hint << "\n";
        }
    }

    void DrawMenuView(const MenuView& view) {
        ClearScreen();

        cout << "=========== " << view.title << " ===========\n\n";

        if (!view.subtitle.empty()) {
            cout << view.subtitle << "\n\n";
        }

        for (size_t i = 0; i < view.items.size(); ++i) {
            PrintSingleMenuItem(view.items[i]);
        }

        if (!view.message.empty()) {
            cout << "\n" << view.message << "\n";
        }

        if (!view.footerHint.empty()) {
            cout << "\n" << view.footerHint << "\n";
        }
    }

    void ApplyMenuSettingsToGame(MenuContext& menu, GameSession& game) {
        game.settings.soundEnabled = menu.appSettings.soundEnabled;
        game.settings.musicEnabled = menu.appSettings.musicEnabled;
        game.settings.soundVolume = menu.appSettings.soundVolume;
        game.settings.musicVolume = menu.appSettings.musicVolume;
        game.settings.language = menu.appSettings.language;

        if (game.settings.gameMode == GameMode::PVE) {
            game.playerO.name = GetBotName(game.settings.aiDifficulty);
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
            message = "Save successful to slot " + to_string(slotIndex);
            return true;
        }

        message = "Save failed";
        return false;
    }

    bool LoadGameFromSlot(GameSession& game, int slotIndex, string& message) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = "This slot is empty";
            return false;
        }

        if (!LoadGameFromFile(game, path)) {
            message = "Load failed";
            return false;
        }

        game.screen = ScreenState::Playing;
        game.isPaused = false;

        if (game.settings.gameMode == GameMode::PVE) {
            game.playerO.name = GetBotName(game.settings.aiDifficulty);
        }

        message = "Load successful";
        return true;
    }

    bool RenameSaveSlotMetadata(int slotIndex, const string& newName, string& message) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = "This slot is empty";
            return false;
        }

        GameSession temp;
        if (!LoadGameFromFile(temp, path)) {
            message = "Cannot read save to rename";
            return false;
        }

        temp.currentSaveName = newName.empty() ? ("slot" + to_string(slotIndex)) : newName;

        if (!SaveGameToFile(temp, path)) {
            message = "Rename save failed";
            return false;
        }

        message = "Renamed save in slot " + to_string(slotIndex);
        return true;
    }

    bool DeleteSaveSlotFile(int slotIndex, string& message) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = "This slot is empty";
            return false;
        }

        if (!DeleteSaveFile(path)) {
            message = "Delete save failed";
            return false;
        }

        message = "Deleted save in slot " + to_string(slotIndex);
        return true;
    }

    void AskPlayerNamesForNewGame(MenuCommand& command) {
        string defaultX = command.text.empty() ? GetDefaultPlayer1Name() : command.text;
        string defaultO = command.extraText.empty() ? GetDefaultPlayer2Name() : command.extraText;

        string playerX = PromptLine("NEW GAME", "Enter Player 1 name", defaultX);

        command.text = playerX;

        if (command.settings.gameMode == GameMode::PVE) {
            command.extraText = GetBotName(command.settings.aiDifficulty);
        }
        else {
            string playerO = PromptLine("NEW GAME", "Enter Player 2 name", defaultO);
            command.extraText = playerO;
        }
    }

    void HandleSaveCommand(MenuContext& menu, GameSession& game, MenuCommand& command) {
        string defaultSaveName = command.text.empty()
            ? BuildSuggestedSaveName(game)
            : command.text;

        string saveName = PromptLine("SAVE GAME", "Enter save name", defaultSaveName);

        string message;
        SaveGameToSlot(game, command.slotIndex, saveName, message);

        UpdateMenuSaveNameDraft(menu, saveName);
        SetMenuStatusMessage(menu, message);
        RefreshMenuSaveSlotsFromFiles(menu);
    }

    void HandleRenameCommand(MenuContext& menu, MenuCommand& command) {
        string currentName = command.text;
        if (currentName.empty()) {
            currentName = "slot" + to_string(command.slotIndex);
        }

        string newName = PromptLine("RENAME SAVE", "Enter new save name", currentName);

        string message;
        RenameSaveSlotMetadata(command.slotIndex, newName, message);

        UpdateMenuRenameDraft(menu, newName);
        SetMenuStatusMessage(menu, message);
        RefreshMenuSaveSlotsFromFiles(menu);
    }

    void HandleDeleteCommand(MenuContext& menu, MenuCommand& command) {
        bool accepted = PromptYesNo(
            "DELETE SAVE",
            "Are you sure you want to delete slot " + to_string(command.slotIndex) + "?"
        );

        if (!accepted) {
            SetMenuStatusMessage(menu, "Delete canceled");
            return;
        }

        string message;
        DeleteSaveSlotFile(command.slotIndex, message);
        SetMenuStatusMessage(menu, message);
        RefreshMenuSaveSlotsFromFiles(menu);
    }

    InGameFlowAction RunInGameMenuLoop(MenuContext& menu, GameSession& game) {
        while (true) {
            RefreshMenuContextForCurrentGame(menu, game);

            MenuView view = BuildMenuView(menu);
            DrawMenuView(view);

            MenuInput input = ReadMenuInputFromKeyboard();
            MenuCommand command = HandleMenuInput(menu, input);

            switch (command.type) {
            case MenuCommandType::None:
                break;

            case MenuCommandType::ApplySettings:
                ApplyMenuSettingsToGame(menu, game);
                SetMenuStatusMessage(menu, "Settings updated");
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
            ostringstream oss;
            oss << "BOT moved to (" << aiMove.row << ", " << aiMove.col << ")";
            message = oss.str();
        }
    }

    void RunGameLoop(GameSession& game, MenuContext& menu) {
        Position cursor = FindDefaultCursor(game);
        string message;

        SyncMenuSettingsFromGame(menu, game);

        while (true) {
            if (IsBotTurn(game)) {
                RunBotMove(game, message);
                // GIU NGUYEN CURSOR, KHONG NHAY VE GOC TRAI
            }

            if (game.result != GameResult::InProgress) {
                SetMenuWinnerDisplayName(menu, GetWinnerDisplayName(game));
                OpenResultMenu(menu, game.result);

                InGameFlowAction action = RunInGameMenuLoop(menu, game);

                if (action == InGameFlowAction::RestartMatch) {
                    ResetCurrentMatch(game);
                    ApplyMenuSettingsToGame(menu, game);
                    cursor = FindDefaultCursor(game);
                    message = "Match restarted";
                    continue;
                }

                if (action == InGameFlowAction::BackToMainMenu) {
                    return;
                }

                continue;
            }

            DrawGameScreen(game, cursor, message);

            int key = ReadKeyUpper();

            if (key == 'W') {
                if (cursor.row > 0) cursor.row--;
            }
            else if (key == 'S') {
                if (cursor.row + 1 < GetBoardSize(game)) cursor.row++;
            }
            else if (key == 'A') {
                if (cursor.col > 0) cursor.col--;
            }
            else if (key == 'D') {
                if (cursor.col + 1 < GetBoardSize(game)) cursor.col++;
            }
            else if (key == 13) {
                ActionResult result = PlaceCurrentTurn(game, cursor);

                if (result == ActionResult::Success) {
                    message = "Placed successfully";
                }
                else {
                    // Danh vao o da co quan thi KHONG LAM GI CA
                    // Giữ nguyên cursor, không đổi message, không đổi lượt
                }
            }
            else if (key == 'P' || key == 27) {
                OpenPauseMenu(menu);

                InGameFlowAction action = RunInGameMenuLoop(menu, game);

                if (action == InGameFlowAction::ContinuePlaying) {
                    message = "Continue game";
                }
                else if (action == InGameFlowAction::RestartMatch) {
                    ResetCurrentMatch(game);
                    ApplyMenuSettingsToGame(menu, game);
                    cursor = FindDefaultCursor(game);
                    message = "Match restarted";
                }
                else if (action == InGameFlowAction::BackToMainMenu) {
                    return;
                }
            }
        }
    }

} // namespace

int main() {
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
            SetMenuStatusMessage(menu, "Settings updated");
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