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

    string NormalizeSaveDirectory() {
        string dir = config::DEFAULT_SAVE_DIRECTORY;
        while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) {
            dir.pop_back();
        }
        if (dir.empty()) dir = "saves";
        return dir;
    }
    //long 24120373
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

    HANDLE GetConsoleHandle() {
        return GetStdHandle(STD_OUTPUT_HANDLE);
    }

    void MoveConsoleCursorToHome() {
        COORD home;
        home.X = 0;
        home.Y = 0;
        SetConsoleCursorPosition(GetConsoleHandle(), home);
    }

    void ClearConsoleBuffer() {
        HANDLE handle = GetConsoleHandle();
        CONSOLE_SCREEN_BUFFER_INFO info;
        GetConsoleScreenBufferInfo(handle, &info);

        DWORD cellCount = (DWORD)(info.dwSize.X * info.dwSize.Y);
        DWORD written = 0;
        COORD home;
        home.X = 0;
        home.Y = 0;

        FillConsoleOutputCharacterA(handle, ' ', cellCount, home, &written);
        FillConsoleOutputAttribute(handle, info.wAttributes, cellCount, home, &written);
        SetConsoleCursorPosition(handle, home);
    }

    void HideConsoleCursor() {
        HANDLE handle = GetConsoleHandle();
        CONSOLE_CURSOR_INFO cursorInfo;
        cursorInfo.dwSize = 1;
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(handle, &cursorInfo);
    }

    void PresentScreenBuffer(const string& buffer) {
        ClearConsoleBuffer();
        MoveConsoleCursorToHome();
        cout << buffer;
        cout.flush();
    }

    string PromptLine(
        Language language,
        const string& vietnameseTitle,
        const string& englishTitle,
        const string& vietnamesePrompt,
        const string& englishPrompt,
        const string& defaultValue
    ) {
        ClearConsoleBuffer();

        cout << SelectText(language, vietnameseTitle, englishTitle) << "\n\n";
        cout << SelectText(language, vietnamesePrompt, englishPrompt);

        if (!defaultValue.empty()) {
            cout << " [" << defaultValue << "]";
        }

        cout << "\n> ";

        string line;
        getline(cin, line);

        if (line.empty()) return defaultValue;
        return line;
    }

    bool PromptYesNo(
        Language language,
        const string& vietnameseTitle,
        const string& englishTitle,
        const string& vietnameseQuestion,
        const string& englishQuestion
    ) {
        ClearConsoleBuffer();

        cout << SelectText(language, vietnameseTitle, englishTitle) << "\n\n";
        cout << SelectText(language, vietnameseQuestion, englishQuestion) << "\n";
        cout << SelectText(language, "Nhan Y de dong y, phim bat ky de huy.\n", "Press Y to confirm, any other key to cancel.\n");

        int key = ReadKeyUpper();
        return key == 'Y';
    }

    char CellToChar(CellState cell) {
        if (cell == CellState::X) return 'X';
        if (cell == CellState::O) return 'O';
        return '.';
    }

    string ResultToText(GameResult result, Language language) {
        return GetDisplayText(result, language);
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

    void AppendBoardToStream(ostringstream& oss, const GameSession& game, const Position& cursor) {
        int n = GetBoardSize(game);

        oss << "     ";
        for (int c = 0; c < n; ++c) {
            oss << setw(3) << c;
        }
        oss << "\n";

        for (int r = 0; r < n; ++r) {
            oss << setw(3) << r << "  ";
            for (int c = 0; c < n; ++c) {
                char ch = CellToChar(GetCell(game, Position(r, c)));

                if (r == cursor.row && c == cursor.col) {
                    oss << "[" << ch << "]";
                }
                else {
                    oss << " " << ch << " ";
                }
            }
            oss << "\n";
        }
    }

    string BuildGameScreenBuffer(const GameSession& game, const Position& cursor, const string& message) {
        const Language lang = game.settings.language;

        ostringstream oss;
        oss << "=========== " << SelectText(lang, "CARO CONSOLE", "CARO CONSOLE") << " ===========\n\n";
        oss << left;
        oss << setw(12) << SelectText(lang, "Che do", "Mode") << ": " << GetDisplayText(game.settings.gameMode, lang) << "\n";
        oss << setw(12) << SelectText(lang, "Luat", "Rule") << ": " << GetDisplayText(game.settings.ruleMode, lang) << "\n";
        oss << setw(12) << "Player X" << ": " << game.playerX.name << "\n";
        oss << setw(12) << "Player O" << ": " << game.playerO.name << "\n";
        oss << setw(12) << SelectText(lang, "Luot", "Turn") << ": " << ToString(game.currentTurn) << "\n";
        oss << setw(12) << SelectText(lang, "Ket qua", "Result") << ": " << ResultToText(game.result, lang) << "\n";
        oss << setw(12) << SelectText(lang, "So nuoc", "Moves") << ": " << game.moveCount << "\n";
        oss << setw(12) << SelectText(lang, "Am thanh", "Sound") << ": " << FormatOnOff(game.settings.soundEnabled, lang) << " (" << game.settings.soundVolume << "%)\n";
        oss << setw(12) << SelectText(lang, "Nhac", "Music") << ": " << FormatOnOff(game.settings.musicEnabled, lang) << " (" << game.settings.musicVolume << "%)\n";
        oss << setw(12) << SelectText(lang, "Ngon ngu", "Language") << ": " << GetDisplayText(game.settings.language, lang) << "\n";

        if (!game.currentSaveName.empty()) {
            oss << setw(12) << SelectText(lang, "Ten save", "Save name") << ": " << game.currentSaveName << "\n";
        }

        if (!message.empty()) {
            oss << setw(12) << SelectText(lang, "Thong bao", "Message") << ": " << message << "\n";
        }

        oss << "\n";
        AppendBoardToStream(oss, game, cursor);

        oss << "\n===== " << SelectText(lang, "DIEU KHIEN", "CONTROLS") << " =====\n";
        oss << "W/A/S/D : " << SelectText(lang, "Di chuyen", "Move") << "\n";
        oss << "Enter   : " << SelectText(lang, "Dat co / xac nhan", "Place piece / confirm") << "\n";
        oss << "P / ESC : " << SelectText(lang, "Mo menu tam dung", "Open pause menu") << "\n";
        oss << SelectText(lang, "Trong menu: W/S de chon, A/D de doi gia tri, Enter de xac nhan", "In menu: W/S to move, A/D to change value, Enter to confirm") << "\n";

        return oss.str();
    }

    void DrawGameScreen(const GameSession& game, const Position& cursor, const string& message) {
        PresentScreenBuffer(BuildGameScreenBuffer(game, cursor, message));
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

    void AppendSingleMenuItem(ostringstream& oss, const MenuItemView& item) {
        string prefix = item.selected ? " > " : "   ";
        string disabled = item.enabled ? "" : "[Locked] ";

        oss << prefix << disabled << item.label;

        if (!item.value.empty() && item.kind != MenuItemKind::SaveSlot) {
            oss << " : " << item.value;
        }

        oss << "\n";

        if (item.selected && !item.hint.empty()) {
            oss << "     " << item.hint << "\n";
        }
    }

    string BuildMenuScreenBuffer(const MenuView& view) {
        ostringstream oss;

        oss << "=========== " << view.title << " ===========\n\n";

        if (!view.subtitle.empty()) {
            oss << view.subtitle << "\n\n";
        }

        for (size_t i = 0; i < view.items.size(); ++i) {
            AppendSingleMenuItem(oss, view.items[i]);
        }

        if (!view.message.empty()) {
            oss << "\n" << view.message << "\n";
        }

        if (!view.footerHint.empty()) {
            oss << "\n" << view.footerHint << "\n";
        }

        return oss.str();
    }

    void DrawMenuView(const MenuView& view) {
        PresentScreenBuffer(BuildMenuScreenBuffer(view));
    }

    void ApplyMenuSettingsToGame(MenuContext& menu, GameSession& game) {
        game.settings.soundEnabled = menu.appSettings.soundEnabled;
        game.settings.musicEnabled = menu.appSettings.musicEnabled;
        game.settings.soundVolume = menu.appSettings.soundVolume;
        game.settings.musicVolume = menu.appSettings.musicVolume;
        game.settings.language = menu.appSettings.language;

        if (game.settings.gameMode == GameMode::PVE) {
            game.playerO.name = GetBotDisplayName(game.settings.aiDifficulty, game.settings.language);
        }
    }

    void SyncMenuSettingsFromGame(MenuContext& menu, const GameSession& game) {
        menu.appSettings = game.settings;
        SyncNewGameDraftFromAppSettings(menu);
        UpdateMenuSaveNameDraft(menu, BuildSuggestedSaveName(game));
        UpdateMenuNewGameNames(menu, game.playerX.name, game.playerO.name);
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
            message = SelectText(
                game.settings.language,
                "Luu thanh cong vao slot " + to_string(slotIndex),
                "Saved successfully to slot " + to_string(slotIndex)
            );
            return true;
        }

        message = SelectText(game.settings.language, "Luu that bai", "Save failed");
        return false;
    }

    bool LoadGameFromSlot(GameSession& game, int slotIndex, Language currentLanguage, string& message) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = SelectText(currentLanguage, "Slot nay dang trong", "This slot is empty");
            return false;
        }

        if (!LoadGameFromFile(game, path)) {
            message = SelectText(currentLanguage, "Tai that bai", "Load failed");
            return false;
        }

        game.screen = ScreenState::Playing;
        game.isPaused = false;
        message = SelectText(game.settings.language, "Tai thanh cong", "Loaded successfully");
        return true;
    }

    bool RenameSaveSlotMetadata(int slotIndex, const string& newName, Language language, string& message) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = SelectText(language, "Slot nay dang trong", "This slot is empty");
            return false;
        }

        GameSession temp;
        if (!LoadGameFromFile(temp, path)) {
            message = SelectText(language, "Khong doc duoc save de doi ten", "Cannot read save to rename");
            return false;
        }

        temp.currentSaveName = newName.empty() ? ("slot" + to_string(slotIndex)) : newName;

        if (!SaveGameToFile(temp, path)) {
            message = SelectText(language, "Doi ten save that bai", "Rename save failed");
            return false;
        }

        message = SelectText(
            language,
            "Da doi ten save o slot " + to_string(slotIndex),
            "Renamed save in slot " + to_string(slotIndex)
        );
        return true;
    }

    bool DeleteSaveSlotFile(int slotIndex, Language language, string& message) {
        string path = BuildSavePath(slotIndex);

        if (!FileExists(path)) {
            message = SelectText(language, "Slot nay dang trong", "This slot is empty");
            return false;
        }

        if (!DeleteSaveFile(path)) {
            message = SelectText(language, "Xoa save that bai", "Delete save failed");
            return false;
        }

        message = SelectText(
            language,
            "Da xoa save o slot " + to_string(slotIndex),
            "Deleted save in slot " + to_string(slotIndex)
        );
        return true;
    }

    void AskPlayerNamesForNewGame(MenuCommand& command) {
        const Language lang = command.settings.language;

        string defaultX = command.text.empty()
            ? string(config::DEFAULT_PLAYER_X_NAME)
            : command.text;

        string playerX = PromptLine(
            lang,
            "CHOI MOI",
            "NEW GAME",
            (command.settings.gameMode == GameMode::PVE) ? "Nhap ten nguoi choi" : "Nhap ten nguoi choi X",
            (command.settings.gameMode == GameMode::PVE) ? "Enter player name" : "Enter Player X name",
            defaultX
        );

        command.text = playerX;

        if (command.settings.gameMode == GameMode::PVE) {
            command.extraText = GetBotDisplayName(command.settings.aiDifficulty, lang);
        }
        else {
            string defaultO = command.extraText.empty()
                ? string(config::DEFAULT_PLAYER_O_NAME)
                : command.extraText;

            string playerO = PromptLine(
                lang,
                "CHOI MOI",
                "NEW GAME",
                "Nhap ten nguoi choi O",
                "Enter Player O name",
                defaultO
            );

            command.extraText = playerO;
        }
    }

    void HandleSaveCommand(MenuContext& menu, GameSession& game, MenuCommand& command) {
        const Language lang = menu.appSettings.language;

        string defaultSaveName = command.text.empty()
            ? BuildSuggestedSaveName(game)
            : command.text;

        string saveName = PromptLine(
            lang,
            "LUU GAME",
            "SAVE GAME",
            "Nhap ten save",
            "Enter save name",
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
            lang,
            "DOI TEN SAVE",
            "RENAME SAVE",
            "Nhap ten moi cho save",
            "Enter new save name",
            currentName
        );

        string message;
        RenameSaveSlotMetadata(command.slotIndex, newName, lang, message);

        UpdateMenuRenameDraft(menu, newName);
        SetMenuStatusMessage(menu, message);
        RefreshMenuSaveSlotsFromFiles(menu);
    }

    void HandleDeleteCommand(MenuContext& menu, MenuCommand& command) {
        const Language lang = menu.appSettings.language;

        bool accepted = PromptYesNo(
            lang,
            "XOA SAVE",
            "DELETE SAVE",
            "Ban co chac muon xoa slot " + to_string(command.slotIndex) + " khong?",
            "Are you sure you want to delete slot " + to_string(command.slotIndex) + "?"
        );

        if (!accepted) {
            SetMenuStatusMessage(menu, SelectText(lang, "Da huy xoa save", "Delete canceled"));
            return;
        }

        string message;
        DeleteSaveSlotFile(command.slotIndex, lang, message);
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
                SetMenuStatusMessage(
                    menu,
                    SelectText(menu.appSettings.language, "Da cap nhat cai dat", "Settings updated")
                );
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
                OpenMainMenuScreen(menu);
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
            oss << GetBotDisplayName(game.settings.aiDifficulty, game.settings.language)
                << " "
                << SelectText(game.settings.language, "danh vao", "moved to")
                << " (" << aiMove.row << ", " << aiMove.col << ")";
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
                cursor = FindDefaultCursor(game);
            }

            if (game.result != GameResult::InProgress) {
                OpenResultMenu(menu, game.result);

                InGameFlowAction action = RunInGameMenuLoop(menu, game);

                if (action == InGameFlowAction::RestartMatch) {
                    ResetCurrentMatch(game);
                    ApplyMenuSettingsToGame(menu, game);
                    cursor = FindDefaultCursor(game);
                    message = SelectText(game.settings.language, "Da bat dau lai tran dau", "Match restarted");
                    continue;
                }

                if (action == InGameFlowAction::BackToMainMenu) {
                    return;
                }

                cursor = FindDefaultCursor(game);
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
                    message = SelectText(game.settings.language, "Danh co thanh cong", "Placed successfully");
                }
                else if (result == ActionResult::Occupied) {
                    message = SelectText(game.settings.language, "O nay da duoc danh", "This cell is already occupied");
                }
                else if (result == ActionResult::OutOfBounds) {
                    message = SelectText(game.settings.language, "Vi tri ngoai ban co", "Position is outside the board");
                }
                else {
                    message = SelectText(game.settings.language, "Khong the danh nuoc nay", "Cannot place this move");
                }
            }
            else if (key == 'P' || key == 27) {
                OpenPauseMenu(menu);

                InGameFlowAction action = RunInGameMenuLoop(menu, game);

                if (action == InGameFlowAction::ContinuePlaying) {
                    message = SelectText(game.settings.language, "Tiep tuc game", "Continue game");
                }
                else if (action == InGameFlowAction::RestartMatch) {
                    ResetCurrentMatch(game);
                    ApplyMenuSettingsToGame(menu, game);
                    cursor = FindDefaultCursor(game);
                    message = SelectText(game.settings.language, "Da bat dau lai tran dau", "Match restarted");
                }
                else if (action == InGameFlowAction::BackToMainMenu) {
                    return;
                }
            }
        }
    }

} // namespace

int main() {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    EnsureSaveDirectory();
    HideConsoleCursor();

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
            SetMenuStatusMessage(
                menu,
                SelectText(menu.appSettings.language, "Da cap nhat cai dat", "Settings updated")
            );
            break;

        case MenuCommandType::RequestRenameSlot:
            HandleRenameCommand(menu, command);
            break;

        case MenuCommandType::RequestDeleteSlot:
            HandleDeleteCommand(menu, command);
            break;

        case MenuCommandType::RequestLoadSlot: {
            string message;
            if (LoadGameFromSlot(game, command.slotIndex, menu.appSettings.language, message)) {
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

    ClearConsoleBuffer();
    cout << SelectText(menu.appSettings.language, "Tam biet!\n", "Goodbye!\n");
    return 0;
}