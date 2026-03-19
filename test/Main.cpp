#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <conio.h>
#include <direct.h>

#define NOMINMAX
#include <windows.h>

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

    struct HoldKeyState {
        bool wasDown;
        ULONGLONG nextRepeatAt;

        HoldKeyState() : wasDown(false), nextRepeatAt(0) {}
    };

    struct EdgeKeyState {
        bool wasDown;
        EdgeKeyState() : wasDown(false) {}
    };

    string g_lastRawFrame;
    int g_lastRenderedLineCount = 0;

    // =====================================================
    // Console helpers
    // =====================================================

    HANDLE GetConsoleHandle() {
        static HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        return h;
    }

    void SetCursorVisible(bool visible) {
        CONSOLE_CURSOR_INFO info;
        info.dwSize = 20;
        info.bVisible = visible ? TRUE : FALSE;
        SetConsoleCursorInfo(GetConsoleHandle(), &info);
    }

    void MoveCursorHome() {
        COORD home = { 0, 0 };
        SetConsoleCursorPosition(GetConsoleHandle(), home);
    }

    void ClearConsoleHard() {
        HANDLE h = GetConsoleHandle();

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(h, &csbi)) return;

        DWORD cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
        DWORD written = 0;
        COORD home = { 0, 0 };

        FillConsoleOutputCharacterA(h, ' ', cellCount, home, &written);
        FillConsoleOutputAttribute(h, csbi.wAttributes, cellCount, home, &written);
        SetConsoleCursorPosition(h, home);
    }

    void ResetRenderCache() {
        g_lastRawFrame.clear();
        g_lastRenderedLineCount = 0;
    }

    int GetConsoleWidth() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(GetConsoleHandle(), &csbi)) {
            return 120;
        }

        int width = (int)csbi.srWindow.Right - (int)csbi.srWindow.Left + 1;
        if (width <= 0) width = 120;
        return width;
    }

    vector<string> SplitLinesKeepEmpty(const string& text) {
        vector<string> lines;
        string current;

        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\n') {
                lines.push_back(current);
                current.clear();
            }
            else if (text[i] != '\r') {
                current.push_back(text[i]);
            }
        }

        lines.push_back(current);
        return lines;
    }

    string NormalizeFrameForConsole(const string& rawFrame) {
        int width = GetConsoleWidth();
        vector<string> lines = SplitLinesKeepEmpty(rawFrame);

        int lineCount = (int)lines.size();
        int finalLineCount = (lineCount > g_lastRenderedLineCount)
            ? lineCount
            : g_lastRenderedLineCount;

        ostringstream out;

        for (int i = 0; i < finalLineCount; ++i) {
            string line = (i < lineCount ? lines[i] : "");

            if ((int)line.size() > width) {
                line = line.substr(0, width);
            }
            else if ((int)line.size() < width) {
                line.append(width - (int)line.size(), ' ');
            }

            out << line;
            if (i + 1 < finalLineCount) {
                out << '\n';
            }
        }

        g_lastRenderedLineCount = lineCount;
        return out.str();
    }

    void PresentFrame(const string& rawFrame) {
        if (rawFrame == g_lastRawFrame) {
            return;
        }

        string normalized = NormalizeFrameForConsole(rawFrame);

        MoveCursorHome();

        DWORD written = 0;
        WriteConsoleA(
            GetConsoleHandle(),
            normalized.c_str(),
            (DWORD)normalized.size(),
            &written,
            NULL
        );

        g_lastRawFrame = rawFrame;
    }

    // =====================================================
    // File / save helpers
    // =====================================================

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

    // =====================================================
    // Input helpers
    // =====================================================

    int ReadKeyUpper() {
        int ch = _getch();

        if (ch == 13) return 13;
        if (ch == 27) return 27;

        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
        return ch;
    }

    bool IsVirtualKeyDown(int vk) {
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    void ClearPendingConsoleInputKeys() {
        while (_kbhit()) {
            _getch();
        }
    }

    void WaitUntilGameplayKeysReleased() {
        while (IsVirtualKeyDown('W') ||
            IsVirtualKeyDown('A') ||
            IsVirtualKeyDown('S') ||
            IsVirtualKeyDown('D') ||
            IsVirtualKeyDown('P') ||
            IsVirtualKeyDown(VK_RETURN) ||
            IsVirtualKeyDown(VK_ESCAPE)) {
            Sleep(10);
        }
    }

    bool ConsumeHoldKey(
        HoldKeyState& state,
        bool isDown,
        ULONGLONG now,
        ULONGLONG firstDelayMs,
        ULONGLONG repeatDelayMs
    ) {
        if (!isDown) {
            state.wasDown = false;
            state.nextRepeatAt = 0;
            return false;
        }

        if (!state.wasDown) {
            state.wasDown = true;
            state.nextRepeatAt = now + firstDelayMs;
            return true;
        }

        if (now >= state.nextRepeatAt) {
            state.nextRepeatAt = now + repeatDelayMs;
            return true;
        }

        return false;
    }

    bool ConsumeEdgeKey(EdgeKeyState& state, bool isDown) {
        if (isDown && !state.wasDown) {
            state.wasDown = true;
            return true;
        }

        if (!isDown) {
            state.wasDown = false;
        }

        return false;
    }

    void ResetGameplayKeyStates(
        HoldKeyState& upKey,
        HoldKeyState& downKey,
        HoldKeyState& leftKey,
        HoldKeyState& rightKey,
        EdgeKeyState& enterKey,
        EdgeKeyState& pauseKey,
        EdgeKeyState& escKey
    ) {
        upKey = HoldKeyState();
        downKey = HoldKeyState();
        leftKey = HoldKeyState();
        rightKey = HoldKeyState();
        enterKey = EdgeKeyState();
        pauseKey = EdgeKeyState();
        escKey = EdgeKeyState();
    }

    // =====================================================
    // Prompt helpers
    // =====================================================

    string PromptLine(const string& title, const string& prompt, const string& defaultValue) {
        ResetRenderCache();
        ClearConsoleHard();
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

    bool PromptYesNo(const string& title, const string& question) {
        ResetRenderCache();
        ClearConsoleHard();
        ClearPendingConsoleInputKeys();

        cout << title << "\n\n";
        cout << question << "\n";
        cout << "Nhan Y de dong y, phim bat ky de huy.\n";

        int key = ReadKeyUpper();

        ClearPendingConsoleInputKeys();
        return key == 'Y';
    }

    // =====================================================
    // Game text helpers
    // =====================================================

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

    bool IsWinningCell(const GameSession& game, int row, int col) {
        for (size_t i = 0; i < game.winningLine.size(); ++i) {
            if (game.winningLine[i].row == row && game.winningLine[i].col == col) {
                return true;
            }
        }
        return false;
    }

    void AppendBoardToStream(
        ostream& out,
        const GameSession& game,
        const Position& cursor,
        bool showWinningLine
    ) {
        int n = GetBoardSize(game);

        out << "     ";
        for (int c = 0; c < n; ++c) {
            out << setw(3) << c;
        }
        out << "\n";

        for (int r = 0; r < n; ++r) {
            out << setw(3) << r << "  ";
            for (int c = 0; c < n; ++c) {
                char ch = CellToChar(GetCell(game, Position(r, c)));

                bool isCursor = (game.result == GameResult::InProgress &&
                    r == cursor.row && c == cursor.col);

                bool isWinning = (showWinningLine && IsWinningCell(game, r, c));

                if (isCursor) {
                    out << "[" << ch << "]";
                }
                else if (isWinning) {
                    out << "{" << ch << "}";
                }
                else {
                    out << " " << ch << " ";
                }
            }
            out << "\n";
        }
    }

    void AppendGameHeaderToStream(ostream& out, const GameSession& game, const string& message) {
        out << "=========== CARO CONSOLE ===========\n\n";
        out << "Mode      : " << ToString(game.settings.gameMode) << "\n";
        out << "Rule      : " << ToString(game.settings.ruleMode) << "\n";
        out << "Player X  : " << game.playerX.name << "\n";
        out << "Player O  : " << game.playerO.name << "\n";
        out << "Turn      : " << ToString(game.currentTurn) << "\n";
        out << "Result    : " << ResultToText(game.result) << "\n";
        out << "Moves     : " << game.moveCount << "\n";
        out << "Sound     : " << (game.settings.soundEnabled ? "ON" : "OFF")
            << " (" << game.settings.soundVolume << "%)\n";
        out << "Music     : " << (game.settings.musicEnabled ? "ON" : "OFF")
            << " (" << game.settings.musicVolume << "%)\n";
        out << "Language  : " << ToString(game.settings.language) << "\n";

        if (!game.currentSaveName.empty()) {
            out << "Save name : " << game.currentSaveName << "\n";
        }

        if (!message.empty()) {
            out << "Message   : " << message << "\n";
        }

        out << "\n";
    }

    void AppendControlsToStream(ostream& out) {
        out << "\n===== CONTROLS =====\n";
        out << "W/A/S/D : Move\n";
        out << "Enter   : Place piece / confirm\n";
        out << "P / ESC : Pause menu\n";
        out << "In menu : W/S move, A/D change, Enter confirm\n";
    }

    string BuildGameScreenText(
        const GameSession& game,
        const Position& cursor,
        const string& message,
        bool showWinningLine
    ) {
        ostringstream out;
        AppendGameHeaderToStream(out, game, message);
        AppendBoardToStream(out, game, cursor, showWinningLine);
        AppendControlsToStream(out);
        return out.str();
    }

    void DrawGameScreen(
        const GameSession& game,
        const Position& cursor,
        const string& message,
        bool showWinningLine = true
    ) {
        PresentFrame(BuildGameScreenText(game, cursor, message, showWinningLine));
    }

    void PlayWinAnimationBeforeResultMenu(
        const GameSession& game,
        const Position& cursor,
        const string& message
    ) {
        if ((game.result != GameResult::XWin && game.result != GameResult::OWin) ||
            game.winningLine.empty()) {
            return;
        }

        const DWORD kFrameDelayMs = 120;
        const int kFrameCount = 10;

        for (int i = 0; i < kFrameCount; ++i) {
            bool visible = (i % 2 == 0);
            DrawGameScreen(game, cursor, message, visible);
            Sleep(kFrameDelayMs);
        }

        DrawGameScreen(game, cursor, message, true);
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

    // =====================================================
    // Menu render/input helpers
    // =====================================================

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

    void AppendSingleMenuItemToStream(ostream& out, const MenuItemView& item) {
        string prefix = item.selected ? " > " : "   ";
        string disabled = item.enabled ? "" : "[Locked] ";

        out << prefix << disabled << item.label;

        if (!item.value.empty() && item.kind != MenuItemKind::SaveSlot) {
            out << " : " << item.value;
        }

        out << "\n";

        if (item.selected && !item.hint.empty()) {
            out << "     " << item.hint << "\n";
        }
    }

    string BuildMenuViewText(const MenuView& view) {
        ostringstream out;

        out << "=========== " << view.title << " ===========\n\n";

        if (!view.subtitle.empty()) {
            out << view.subtitle << "\n\n";
        }

        for (size_t i = 0; i < view.items.size(); ++i) {
            AppendSingleMenuItemToStream(out, view.items[i]);
        }

        if (!view.message.empty()) {
            out << "\n" << view.message << "\n";
        }

        if (!view.footerHint.empty()) {
            out << "\n" << view.footerHint << "\n";
        }

        return out.str();
    }

    void DrawMenuView(const MenuView& view) {
        PresentFrame(BuildMenuViewText(view));
    }

    // =====================================================
    // Game/menu sync helpers
    // =====================================================

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

    // =====================================================
    // Save/load helpers
    // =====================================================

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

    // =====================================================
    // Menu actions
    // =====================================================

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

        ResetRenderCache();
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

        ResetRenderCache();
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

        ResetRenderCache();
    }

    void HandleDeleteCommand(MenuContext& menu, MenuCommand& command) {
        bool accepted = PromptYesNo(
            "DELETE SAVE",
            "Are you sure you want to delete slot " + to_string(command.slotIndex) + "?"
        );

        if (!accepted) {
            SetMenuStatusMessage(menu, "Delete canceled");
            ResetRenderCache();
            return;
        }

        string message;
        DeleteSaveSlotFile(command.slotIndex, message);
        SetMenuStatusMessage(menu, message);
        RefreshMenuSaveSlotsFromFiles(menu);

        ResetRenderCache();
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
                ResetRenderCache();
                return InGameFlowAction::ContinuePlaying;

            case MenuCommandType::RestartGame:
                ResetRenderCache();
                return InGameFlowAction::RestartMatch;

            case MenuCommandType::BackToMainMenu:
                ResetRenderCache();
                return InGameFlowAction::BackToMainMenu;

            default:
                break;
            }
        }
    }

    // =====================================================
    // Bot / gameplay
    // =====================================================

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

        bool needsRedraw = true;
        bool resultMenuOpenedForThisMatch = false;

        HoldKeyState upKey;
        HoldKeyState downKey;
        HoldKeyState leftKey;
        HoldKeyState rightKey;
        EdgeKeyState enterKey;
        EdgeKeyState pauseKey;
        EdgeKeyState escKey;

        SyncMenuSettingsFromGame(menu, game);
        ResetRenderCache();

        const ULONGLONG firstRepeatDelayMs = 140;
        const ULONGLONG repeatDelayMs = 45;
        const DWORD idleSleepMs = 8;

        while (true) {
            if (IsBotTurn(game)) {
                RunBotMove(game, message);
                needsRedraw = true;
            }

            if (game.result != GameResult::InProgress) {
                if (!resultMenuOpenedForThisMatch) {
                    if (needsRedraw) {
                        DrawGameScreen(game, cursor, message, true);
                        needsRedraw = false;
                    }

                    PlayWinAnimationBeforeResultMenu(game, cursor, message);

                    ResetGameplayKeyStates(
                        upKey, downKey, leftKey, rightKey,
                        enterKey, pauseKey, escKey
                    );

                    WaitUntilGameplayKeysReleased();
                    ClearPendingConsoleInputKeys();

                    SetMenuWinnerDisplayName(menu, GetWinnerDisplayName(game));
                    OpenResultMenu(menu, game.result);

                    resultMenuOpenedForThisMatch = true;
                    ResetRenderCache();
                }

                InGameFlowAction action = RunInGameMenuLoop(menu, game);

                ResetGameplayKeyStates(
                    upKey, downKey, leftKey, rightKey,
                    enterKey, pauseKey, escKey
                );

                WaitUntilGameplayKeysReleased();
                ClearPendingConsoleInputKeys();

                if (action == InGameFlowAction::RestartMatch) {
                    ResetCurrentMatch(game);
                    ApplyMenuSettingsToGame(menu, game);
                    cursor = FindDefaultCursor(game);
                    message = "Match restarted";
                    needsRedraw = true;
                    resultMenuOpenedForThisMatch = false;
                    ResetRenderCache();
                    continue;
                }

                if (action == InGameFlowAction::BackToMainMenu) {
                    return;
                }

                needsRedraw = true;
                ResetRenderCache();
                continue;
            }

            resultMenuOpenedForThisMatch = false;

            if (needsRedraw) {
                DrawGameScreen(game, cursor, message, true);
                needsRedraw = false;
            }

            ULONGLONG now = GetTickCount64();
            bool moved = false;

            if (ConsumeHoldKey(upKey, IsVirtualKeyDown('W'), now, firstRepeatDelayMs, repeatDelayMs)) {
                if (cursor.row > 0) {
                    cursor.row--;
                    moved = true;
                }
            }

            if (ConsumeHoldKey(downKey, IsVirtualKeyDown('S'), now, firstRepeatDelayMs, repeatDelayMs)) {
                if (cursor.row + 1 < GetBoardSize(game)) {
                    cursor.row++;
                    moved = true;
                }
            }

            if (ConsumeHoldKey(leftKey, IsVirtualKeyDown('A'), now, firstRepeatDelayMs, repeatDelayMs)) {
                if (cursor.col > 0) {
                    cursor.col--;
                    moved = true;
                }
            }

            if (ConsumeHoldKey(rightKey, IsVirtualKeyDown('D'), now, firstRepeatDelayMs, repeatDelayMs)) {
                if (cursor.col + 1 < GetBoardSize(game)) {
                    cursor.col++;
                    moved = true;
                }
            }

            if (moved) {
                needsRedraw = true;
            }

            if (ConsumeEdgeKey(enterKey, IsVirtualKeyDown(VK_RETURN))) {
                ActionResult result = PlaceCurrentTurn(game, cursor);

                if (result == ActionResult::Success) {
                    message = "Placed successfully";
                    needsRedraw = true;
                }
            }

            bool pausePressed = false;

            if (ConsumeEdgeKey(pauseKey, IsVirtualKeyDown('P'))) {
                pausePressed = true;
            }

            if (ConsumeEdgeKey(escKey, IsVirtualKeyDown(VK_ESCAPE))) {
                pausePressed = true;
            }

            if (pausePressed) {
                ResetGameplayKeyStates(
                    upKey, downKey, leftKey, rightKey,
                    enterKey, pauseKey, escKey
                );

                WaitUntilGameplayKeysReleased();
                ClearPendingConsoleInputKeys();

                OpenPauseMenu(menu);

                InGameFlowAction action = RunInGameMenuLoop(menu, game);

                ResetGameplayKeyStates(
                    upKey, downKey, leftKey, rightKey,
                    enterKey, pauseKey, escKey
                );

                WaitUntilGameplayKeysReleased();
                ClearPendingConsoleInputKeys();

                if (action == InGameFlowAction::ContinuePlaying) {
                    message = "Continue game";
                    needsRedraw = true;
                    ResetRenderCache();
                }
                else if (action == InGameFlowAction::RestartMatch) {
                    ResetCurrentMatch(game);
                    ApplyMenuSettingsToGame(menu, game);
                    cursor = FindDefaultCursor(game);
                    message = "Match restarted";
                    needsRedraw = true;
                    resultMenuOpenedForThisMatch = false;
                    ResetRenderCache();
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
    EnsureSaveDirectory();
    SetCursorVisible(false);
    ResetRenderCache();
    ClearConsoleHard();

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
                ResetRenderCache();
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
            ResetRenderCache();
            break;
        }

        case MenuCommandType::ExitApplication:
            running = false;
            break;

        default:
            break;
        }
    }

    ResetRenderCache();
    ClearConsoleHard();
    cout << "Goodbye!\n";
    return 0;
}