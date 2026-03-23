#include "ConsoleRenderer.h"
#include "GameAPI.h"
#include "MenuText.h"

#include <windows.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

namespace caro {

    namespace {

        HANDLE GetConsoleHandle() {
            return GetStdHandle(STD_OUTPUT_HANDLE);
        }

        HANDLE GetConsoleInputHandle() {
            return GetStdHandle(STD_INPUT_HANDLE);
        }

        std::string PadRight(const std::string& text, int width) {
            if ((int)text.size() >= width) {
                return text.substr(0, width);
            }
            return text + std::string(width - (int)text.size(), ' ');
        }

        char CellToChar(CellState cell) {
            if (cell == CellState::X) return 'X';
            if (cell == CellState::O) return 'O';
            return '.';
        }

        void PrintDivider(std::ostream& out, int width) {
            out << '+' << std::string(width - 2, '=') << "+\n";
        }

        void PrintThinDivider(std::ostream& out, int width) {
            out << '+' << std::string(width - 2, '-') << "+\n";
        }

        void PrintFramedLine(std::ostream& out, const std::string& text, int width) {
            out << '|' << PadRight(text, width - 2) << "|\n";
        }

        bool ContainsPosition(const std::vector<Position>* cells, int row, int col) {
            if (cells == 0) return false;
            for (size_t i = 0; i < cells->size(); ++i) {
                if ((*cells)[i].row == row && (*cells)[i].col == col) {
                    return true;
                }
            }
            return false;
        }

        void HideCursor() {
            HANDLE out = GetConsoleHandle();
            CONSOLE_CURSOR_INFO info;
            info.dwSize = 1;
            info.bVisible = FALSE;
            SetConsoleCursorInfo(out, &info);
        }

        void ConfigureConsoleInput() {
            HANDLE in = GetConsoleInputHandle();
            DWORD inputMode = 0;

            if (GetConsoleMode(in, &inputMode)) {
                inputMode |= ENABLE_EXTENDED_FLAGS;
                inputMode &= ~ENABLE_QUICK_EDIT_MODE;
                inputMode &= ~ENABLE_MOUSE_INPUT;
                inputMode &= ~ENABLE_WINDOW_INPUT;
                SetConsoleMode(in, inputMode);
            }
        }

        struct ConsoleSize {
            int windowWidth;
            int windowHeight;
            int bufferWidth;
            int bufferHeight;
        };

        ConsoleSize GetConsoleSize() {
            ConsoleSize size;
            size.windowWidth = 120;
            size.windowHeight = 40;
            size.bufferWidth = 120;
            size.bufferHeight = 40;

            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (GetConsoleScreenBufferInfo(GetConsoleHandle(), &csbi)) {
                size.windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
                size.windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
                size.bufferWidth = csbi.dwSize.X;
                size.bufferHeight = csbi.dwSize.Y;
            }

            return size;
        }

        std::vector<std::string> SplitLines(const std::string& text) {
            std::vector<std::string> lines;
            std::string current;

            for (size_t i = 0; i < text.size(); ++i) {
                char ch = text[i];
                if (ch == '\r') continue;

                if (ch == '\n') {
                    lines.push_back(current);
                    current.clear();
                }
                else {
                    current.push_back(ch);
                }
            }

            lines.push_back(current);
            return lines;
        }

        int GetLongestLineLength(const std::vector<std::string>& lines) {
            int longest = 0;
            for (size_t i = 0; i < lines.size(); ++i) {
                int len = (int)lines[i].size();
                if (len > longest) longest = len;
            }
            return longest;
        }

        void EnsureConsoleBuffer(int requiredWidth, int requiredHeight) {
            HANDLE out = GetConsoleHandle();

            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (!GetConsoleScreenBufferInfo(out, &csbi)) return;

            int currentWindowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
            int currentWindowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

            int targetWidth = max(requiredWidth, currentWindowWidth);
            int targetHeight = max(requiredHeight, currentWindowHeight);

            COORD newSize;
            newSize.X = (SHORT)targetWidth;
            newSize.Y = (SHORT)targetHeight;

            if (csbi.dwSize.X != newSize.X || csbi.dwSize.Y != newSize.Y) {
                SetConsoleScreenBufferSize(out, newSize);
            }

            SMALL_RECT rect;
            rect.Left = 0;
            rect.Top = 0;
            rect.Right = (SHORT)(currentWindowWidth - 1);
            rect.Bottom = (SHORT)(currentWindowHeight - 1);
            SetConsoleWindowInfo(out, TRUE, &rect);

            COORD home;
            home.X = 0;
            home.Y = 0;
            SetConsoleCursorPosition(out, home);
        }

        void RenderFrameText(const std::string& frame) {
            HANDLE out = GetConsoleHandle();

            std::vector<std::string> lines = SplitLines(frame);
            int frameWidth = GetLongestLineLength(lines);
            int frameHeight = (int)lines.size();

            ConsoleSize current = GetConsoleSize();

            int renderWidth = max(frameWidth, current.windowWidth);
            int renderHeight = max(frameHeight, current.windowHeight);

            EnsureConsoleBuffer(renderWidth, renderHeight);

            const int screenSize = renderWidth * renderHeight;
            std::string screen(screenSize, ' ');
            std::vector<WORD> attrs(
                screenSize,
                (WORD)(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
            );

            for (int y = 0; y < frameHeight; ++y) {
                const std::string& line = lines[y];
                int copyLen = min((int)line.size(), renderWidth);
                for (int x = 0; x < copyLen; ++x) {
                    screen[y * renderWidth + x] = line[x];
                }
            }

            DWORD written = 0;
            COORD home;
            home.X = 0;
            home.Y = 0;

            WriteConsoleOutputCharacterA(
                out,
                screen.c_str(),
                (DWORD)screen.size(),
                home,
                &written
            );

            WriteConsoleOutputAttribute(
                out,
                &attrs[0],
                (DWORD)attrs.size(),
                home,
                &written
            );

            SetConsoleCursorPosition(out, home);
        }

        int GetMenuRenderWidth() {
            ConsoleSize size = GetConsoleSize();
            return min(max(78, size.windowWidth - 4), 120);
        }

        int GetGameRenderWidth() {
            ConsoleSize size = GetConsoleSize();
            return min(max(88, size.windowWidth - 4), 140);
        }

        void AppendMenuHeader(std::ostream& out, const MenuView& view, int width) {
            PrintDivider(out, width);
            PrintFramedLine(out, " CARO GAME - MENU", width);
            PrintThinDivider(out, width);
            PrintFramedLine(out, " " + view.title, width);

            if (!view.subtitle.empty()) {
                PrintFramedLine(out, " " + view.subtitle, width);
            }

            PrintDivider(out, width);
        }

        void AppendMenuItems(std::ostream& out, const MenuView& view, int width) {
            for (size_t i = 0; i < view.items.size(); ++i) {
                const MenuItemView& item = view.items[i];
                std::ostringstream line;

                line << (item.selected ? " > " : "   ") << item.label;

                if (!item.value.empty()) {
                    line << " : " << item.value;
                }

                if (!item.enabled) {
                    line << " [Locked]";
                }

                PrintFramedLine(out, line.str(), width);

                if (item.selected && !item.hint.empty()) {
                    PrintFramedLine(out, "     " + item.hint, width);
                }
            }
        }

        void AppendMenuFooter(std::ostream& out, const MenuView& view, int width) {
            PrintDivider(out, width);

            if (!view.message.empty()) {
                PrintFramedLine(out, " " + view.message, width);
                PrintThinDivider(out, width);
            }

            if (!view.footerHint.empty()) {
                PrintFramedLine(out, " " + view.footerHint, width);
            }
            else {
                PrintFramedLine(out, " Mui ten / W,A,S,D / I,J,K,L: Di chuyen | Enter: Chon | ESC: Quay lai", width);
            }

            PrintDivider(out, width);
        }

        void AppendGameHeader(std::ostream& out, const GameSession& game, const std::string& message, int width) {
            Language lang = game.settings.language;

            PrintDivider(out, width);
            PrintFramedLine(out, (lang == Language::Vietnamese ? " CARO GAME - TRAN DAU" : " CARO GAME - MATCH"), width);
            PrintThinDivider(out, width);

            std::ostringstream info1;
            info1 << ' '
                << SelectText(lang, "Che do", "Mode") << ": " << GetDisplayText(game.settings.gameMode, lang)
                << " | " << SelectText(lang, "Luat", "Rule") << ": " << GetDisplayText(game.settings.ruleMode, lang)
                << " | " << SelectText(lang, "Luot", "Turn") << ": " << (game.currentTurn == CellState::X ? "X" : "O");
            PrintFramedLine(out, info1.str(), width);

            std::ostringstream info2;
            info2 << ' '
                << SelectText(lang, "Nguoi choi X", "Player X") << ": " << game.playerX.name
                << " | " << SelectText(lang, "Nguoi choi O", "Player O") << ": " << game.playerO.name;
            PrintFramedLine(out, info2.str(), width);

            std::ostringstream info3;
            info3 << ' '
                << SelectText(lang, "Ket qua", "Result") << ": " << GetDisplayText(game.result, lang)
                << " | " << SelectText(lang, "So nuoc", "Moves") << ": " << game.moveCount
                << " | " << SelectText(lang, "Ban co", "Board") << ": "
                << game.settings.boardSize << 'x' << game.settings.boardSize;
            PrintFramedLine(out, info3.str(), width);

            if (!message.empty()) {
                PrintThinDivider(out, width);
                PrintFramedLine(out, " " + SelectText(lang, "Thong bao", "Message") + ": " + message, width);
            }

            PrintDivider(out, width);
        }

        void AppendBoard(
            std::ostream& out,
            const GameSession& game,
            const Position& cursor,
            const std::vector<Position>* highlightedCells,
            bool blinkPhase
        ) {
            int n = GetBoardSize(game);

            out << "\n     ";
            for (int c = 0; c < n; ++c) {
                out << std::setw(3) << c;
            }
            out << "\n";

            for (int r = 0; r < n; ++r) {
                out << std::setw(3) << r << "  ";
                for (int c = 0; c < n; ++c) {
                    char ch = CellToChar(GetCell(game, Position(r, c)));
                    bool isCursor = (r == cursor.row && c == cursor.col);
                    bool isHighlighted = ContainsPosition(highlightedCells, r, c);

                    if (isHighlighted) {
                        if (blinkPhase) {
                            out << '[' << ch << ']';
                        }
                        else {
                            out << '{' << ch << '}';
                        }
                    }
                    else if (isCursor && game.result == GameResult::InProgress) {
                        out << '(' << ch << ')';
                    }
                    else {
                        out << ' ' << ch << ' ';
                    }
                }
                out << "\n";
            }
        }

        void AppendControls(std::ostream& out, Language lang, int width) {
            out << "\n";
            PrintDivider(out, width);
            if (lang == Language::Vietnamese) {
                PrintFramedLine(out, " Mui ten / W,A,S,D / I,J,K,L: Di chuyen | Enter: Dat quan | P/ESC: Menu", width);
            }
            else {
                PrintFramedLine(out, " Arrow / W,A,S,D / I,J,K,L: Move | Enter: Place | P/ESC: Menu", width);
            }
            PrintDivider(out, width);
        }

        void AppendResultPanel(std::ostream& out, const MenuView& view, Language lang, int width) {
            out << "\n";
            PrintDivider(out, width);
            PrintFramedLine(out, (lang == Language::Vietnamese ? " BANG KET QUA" : " RESULT PANEL"), width);
            PrintThinDivider(out, width);
            PrintFramedLine(out, " " + view.title, width);

            if (!view.subtitle.empty()) {
                PrintFramedLine(out, " " + view.subtitle, width);
            }

            PrintDivider(out, width);

            for (size_t i = 0; i < view.items.size(); ++i) {
                const MenuItemView& item = view.items[i];
                std::ostringstream line;
                line << (item.selected ? " > " : "   ") << item.label;
                if (!item.value.empty()) {
                    line << " : " << item.value;
                }
                PrintFramedLine(out, line.str(), width);
            }

            PrintDivider(out, width);

            if (!view.message.empty()) {
                PrintFramedLine(out, " " + view.message, width);
                PrintThinDivider(out, width);
            }

            if (lang == Language::Vietnamese) {
                PrintFramedLine(out, " W/S hoac mui ten de chon | Enter xac nhan", width);
            }
            else {
                PrintFramedLine(out, " W/S or arrows to choose | Enter to confirm", width);
            }

            PrintDivider(out, width);
        }

        bool BuildLineFromStart(
            const GameSession& game,
            Position start,
            CellState symbol,
            int dRow,
            int dCol,
            std::vector<Position>& out
        ) {
            out.clear();

            int n = GetBoardSize(game);
            int row = start.row;
            int col = start.col;

            while (row >= 0 && row < n && col >= 0 && col < n &&
                GetCell(game, Position(row, col)) == symbol) {
                out.push_back(Position(row, col));
                row += dRow;
                col += dCol;
            }

            return (int)out.size() >= config::WIN_LENGTH;
        }

    } // namespace

    void InitializeConsoleUI() {
        SetConsoleOutputCP(65001);
        SetConsoleCP(65001);
        ConfigureConsoleInput();
        HideCursor();
        ClearScreenFast();
    }

    void ClearScreenFast() {
        HANDLE out = GetConsoleHandle();

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(out, &csbi)) {
            return;
        }

        DWORD cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
        DWORD count = 0;

        COORD home;
        home.X = 0;
        home.Y = 0;

        FillConsoleOutputCharacterA(out, ' ', cellCount, home, &count);
        FillConsoleOutputAttribute(out, csbi.wAttributes, cellCount, home, &count);
        SetConsoleCursorPosition(out, home);
    }

    void SleepMs(int ms) {
        if (ms > 0) {
            Sleep((DWORD)ms);
        }
    }

    void DrawMenuViewEnhanced(const MenuView& view) {
        std::ostringstream out;
        int width = GetMenuRenderWidth();

        AppendMenuHeader(out, view, width);
        AppendMenuItems(out, view, width);
        AppendMenuFooter(out, view, width);

        RenderFrameText(out.str());
    }

    void DrawGameScreenEnhanced(
        const GameSession& game,
        const Position& cursor,
        const std::string& message,
        const std::vector<Position>* highlightedCells,
        bool blinkPhase
    ) {
        std::ostringstream out;
        int width = GetGameRenderWidth();

        AppendGameHeader(out, game, message, width);
        AppendBoard(out, game, cursor, highlightedCells, blinkPhase);
        AppendControls(out, game.settings.language, width);

        RenderFrameText(out.str());
    }

    void DrawGameResultScreenEnhanced(
        const GameSession& game,
        const Position& cursor,
        const std::string& message,
        const MenuView& resultView,
        const std::vector<Position>* highlightedCells,
        bool blinkPhase
    ) {
        std::ostringstream out;
        int width = GetGameRenderWidth();

        AppendGameHeader(out, game, message, width);
        AppendBoard(out, game, cursor, highlightedCells, blinkPhase);
        AppendResultPanel(out, resultView, game.settings.language, width);

        RenderFrameText(out.str());
    }

    std::vector<Position> FindWinningLineCells(const GameSession& game) {
        std::vector<Position> line;

        if (game.result != GameResult::XWin && game.result != GameResult::OWin) {
            return line;
        }

        CellState winner = (game.result == GameResult::XWin) ? CellState::X : CellState::O;
        int n = GetBoardSize(game);

        const int directions[4][2] = {
            {0, 1},
            {1, 0},
            {1, 1},
            {1, -1}
        };

        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                if (GetCell(game, Position(r, c)) != winner) continue;

                for (int i = 0; i < 4; ++i) {
                    int prevRow = r - directions[i][0];
                    int prevCol = c - directions[i][1];

                    if (prevRow >= 0 && prevRow < n &&
                        prevCol >= 0 && prevCol < n &&
                        GetCell(game, Position(prevRow, prevCol)) == winner) {
                        continue;
                    }

                    if (BuildLineFromStart(
                        game,
                        Position(r, c),
                        winner,
                        directions[i][0],
                        directions[i][1],
                        line)) {
                        if ((int)line.size() > config::WIN_LENGTH) {
                            line.resize(config::WIN_LENGTH);
                        }
                        return line;
                    }
                }
            }
        }

        line.clear();
        return line;
    }

} // namespace caro