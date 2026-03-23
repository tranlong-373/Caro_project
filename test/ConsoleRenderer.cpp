#include "ConsoleRenderer.h"
#include "GameAPI.h"
#include "MenuText.h"

#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace caro {

    namespace {

        HANDLE HOut() { return GetStdHandle(STD_OUTPUT_HANDLE); }
        HANDLE HIn() { return GetStdHandle(STD_INPUT_HANDLE); }

        void SetCursorVisible(bool visible) {
            CONSOLE_CURSOR_INFO info;
            info.dwSize = 20;
            info.bVisible = visible ? TRUE : FALSE;
            SetConsoleCursorInfo(HOut(), &info);
        }

        void ApplyConsoleInputMode() {
            DWORD mode = 0;
            if (!GetConsoleMode(HIn(), &mode)) return;

            // Tắt QuickEdit cần ENABLE_EXTENDED_FLAGS theo docs
            // đồng thời tắt mouse và window input (window input có thể bật nếu muốn nhận event resize qua ReadConsoleInput).
            mode |= ENABLE_EXTENDED_FLAGS;
            mode &= ~ENABLE_QUICK_EDIT_MODE;
            mode &= ~ENABLE_MOUSE_INPUT;
            mode &= ~ENABLE_WINDOW_INPUT;

            SetConsoleMode(HIn(), mode);
        }

        CONSOLE_SCREEN_BUFFER_INFO GetCSBI() {
            CONSOLE_SCREEN_BUFFER_INFO csbi{};
            GetConsoleScreenBufferInfo(HOut(), &csbi);
            return csbi;
        }

        int ViewWidth(const CONSOLE_SCREEN_BUFFER_INFO& csbi) {
            return (int)csbi.srWindow.Right - (int)csbi.srWindow.Left + 1;
        }
        int ViewHeight(const CONSOLE_SCREEN_BUFFER_INFO& csbi) {
            return (int)csbi.srWindow.Bottom - (int)csbi.srWindow.Top + 1;
        }

        std::string PadRight(const std::string& s, int w) {
            if ((int)s.size() >= w) return s.substr(0, w);
            return s + std::string(w - (int)s.size(), ' ');
        }

        std::vector<std::string> SplitLinesKeepEmpty(const std::string& text) {
            std::vector<std::string> lines;
            std::string cur;
            for (size_t i = 0; i < text.size(); ++i) {
                char ch = text[i];
                if (ch == '\r') continue;
                if (ch == '\n') { lines.push_back(cur); cur.clear(); }
                else { cur.push_back(ch); }
            }
            lines.push_back(cur);
            return lines;
        }

        void NormalizeToViewport(std::vector<std::string>& lines, int width, int height) {
            for (size_t i = 0; i < lines.size(); ++i) {
                lines[i] = PadRight(lines[i], width);
            }

            if ((int)lines.size() < height) {
                lines.resize(height, std::string(width, ' '));
            }
            else if ((int)lines.size() > height) {
                lines.resize(height);
            }
        }

        // One-shot write: dùng CHAR_INFO + WriteConsoleOutput
        void PresentLines(const std::vector<std::string>& normalizedLines) {
            CONSOLE_SCREEN_BUFFER_INFO csbi = GetCSBI();
            const int w = ViewWidth(csbi);
            const int h = ViewHeight(csbi);
            if (w <= 0 || h <= 0) return;

            std::vector<CHAR_INFO> buf((size_t)w * (size_t)h);

            WORD attr = (WORD)(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

            // Fill
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    CHAR_INFO& cell = buf[(size_t)y * (size_t)w + (size_t)x];
                    cell.Char.AsciiChar = ' ';
                    cell.Attributes = attr;
                }
            }

            // Copy lines
            const int copyH = std::min(h, (int)normalizedLines.size());
            for (int y = 0; y < copyH; ++y) {
                const std::string& line = normalizedLines[(size_t)y];
                const int copyW = std::min(w, (int)line.size());
                for (int x = 0; x < copyW; ++x) {
                    buf[(size_t)y * (size_t)w + (size_t)x].Char.AsciiChar = line[(size_t)x];
                }
            }

            SMALL_RECT target = csbi.srWindow; // vẽ đúng viewport hiện tại
            COORD bufSize{ (SHORT)w, (SHORT)h };
            COORD bufCoord{ 0, 0 };

            WriteConsoleOutputA(HOut(), buf.data(), bufSize, bufCoord, &target);
            SetConsoleCursorPosition(HOut(), { csbi.srWindow.Left, csbi.srWindow.Top });
        }

        void PrintBorder(std::ostream& out, int width, char ch = '=') {
            out << '+' << std::string(std::max(0, width - 2), ch) << "+\n";
        }
        void PrintBoxLine(std::ostream& out, int width, const std::string& content) {
            out << '|' << PadRight(content, std::max(0, width - 2)) << "|\n";
        }

        bool ContainsPos(const std::vector<Position>* cells, int r, int c) {
            if (!cells) return false;
            for (size_t i = 0; i < cells->size(); ++i) {
                if ((*cells)[i].row == r && (*cells)[i].col == c) return true;
            }
            return false;
        }

        char CellChar(CellState s) {
            if (s == CellState::X) return 'X';
            if (s == CellState::O) return 'O';
            return '.';
        }

        std::vector<std::string> RenderBoardLines(
            const GameSession& game,
            const Position& cursor,
            const std::vector<Position>* highlighted,
            bool blinkPhase
        ) {
            std::vector<std::string> lines;
            const int n = GetBoardSize(game);

            std::ostringstream header;
            header << "    ";
            for (int c = 0; c < n; ++c) header << std::setw(3) << c;
            lines.push_back(header.str());

            for (int r = 0; r < n; ++r) {
                std::ostringstream row;
                row << std::setw(3) << r << ' ';
                for (int c = 0; c < n; ++c) {
                    char ch = CellChar(GetCell(game, Position(r, c)));
                    bool isCursor = (game.result == GameResult::InProgress && r == cursor.row && c == cursor.col);
                    bool isHi = ContainsPos(highlighted, r, c);

                    if (isHi) {
                        row << (blinkPhase ? '[' : '{') << ch << (blinkPhase ? ']' : '}');
                    }
                    else if (isCursor) {
                        row << '(' << ch << ')';
                    }
                    else {
                        row << ' ' << ch << ' ';
                    }
                }
                lines.push_back(row.str());
            }

            return lines;
        }

        std::vector<std::string> RenderMenuPanelLines(const MenuView& view, int panelWidth) {
            std::vector<std::string> lines;
            std::ostringstream out;

            PrintBorder(out, panelWidth, '=');
            PrintBoxLine(out, panelWidth, " " + view.title);
            if (!view.subtitle.empty()) {
                PrintBorder(out, panelWidth, '-');
                PrintBoxLine(out, panelWidth, " " + view.subtitle);
            }
            PrintBorder(out, panelWidth, '=');

            // Simple scrolling: chỉ hiển thị đúng số dòng có thể thấy
            int selected = -1;
            for (size_t i = 0; i < view.items.size(); ++i) {
                if (view.items[i].selected) { selected = (int)i; break; }
            }

            // Tạm: in toàn bộ item; (nâng cấp scrolling có thể làm sau)
            for (size_t i = 0; i < view.items.size(); ++i) {
                const MenuItemView& it = view.items[i];
                std::ostringstream line;
                line << (it.selected ? " > " : "   ") << it.label;
                if (!it.value.empty() && it.kind != MenuItemKind::SaveSlot) line << " : " << it.value;
                if (!it.enabled) line << " [Locked]";
                PrintBoxLine(out, panelWidth, line.str());
                if (it.selected && !it.hint.empty()) {
                    PrintBoxLine(out, panelWidth, "     " + it.hint);
                }
            }

            PrintBorder(out, panelWidth, '=');

            if (!view.message.empty()) {
                PrintBoxLine(out, panelWidth, " " + view.message);
                PrintBorder(out, panelWidth, '-');
            }

            if (!view.footerHint.empty()) {
                PrintBoxLine(out, panelWidth, " " + view.footerHint);
            }

            PrintBorder(out, panelWidth, '=');

            lines = SplitLinesKeepEmpty(out.str());
            // đảm bảo không có line quá dài
            for (size_t i = 0; i < lines.size(); ++i) {
                lines[i] = PadRight(lines[i], panelWidth);
            }
            return lines;
        }

        std::vector<std::string> RenderGameHeaderLines(const GameSession& game, const std::string& message, int width) {
            std::vector<std::string> lines;
            Language lang = game.settings.language;

            std::ostringstream out;
            PrintBorder(out, width, '=');
            PrintBoxLine(out, width, " CARO GAME");
            PrintBorder(out, width, '-');

            {
                std::ostringstream s;
                s << ' ' << SelectText(lang, "Che do", "Mode") << ": " << GetDisplayText(game.settings.gameMode, lang)
                    << " | " << SelectText(lang, "Luat", "Rule") << ": " << GetDisplayText(game.settings.ruleMode, lang)
                    << " | " << SelectText(lang, "Luot", "Turn") << ": " << (game.currentTurn == CellState::X ? "X" : "O");
                PrintBoxLine(out, width, s.str());
            }
            {
                std::ostringstream s;
                s << ' ' << SelectText(lang, "Nguoi choi X", "Player X") << ": " << game.playerX.name
                    << " | " << SelectText(lang, "Nguoi choi O", "Player O") << ": " << game.playerO.name;
                PrintBoxLine(out, width, s.str());
            }
            {
                std::ostringstream s;
                s << ' ' << SelectText(lang, "Ket qua", "Result") << ": " << GetDisplayText(game.result, lang)
                    << " | " << SelectText(lang, "So nuoc", "Moves") << ": " << game.moveCount
                    << " | " << SelectText(lang, "Ban co", "Board") << ": " << game.settings.boardSize << "x" << game.settings.boardSize;
                PrintBoxLine(out, width, s.str());
            }

            if (!message.empty()) {
                PrintBorder(out, width, '-');
                PrintBoxLine(out, width, " " + SelectText(lang, "Thong bao", "Message") + ": " + message);
            }

            PrintBorder(out, width, '=');
            lines = SplitLinesKeepEmpty(out.str());
            for (size_t i = 0; i < lines.size(); ++i) lines[i] = PadRight(lines[i], width);
            return lines;
        }

        // Ghép panel (side-by-side hoặc vertical) sao cho fit viewport
        std::vector<std::string> ComposeGameWithOptionalPanel(
            const GameSession& game,
            const Position& cursor,
            const std::string& message,
            const MenuView* panelView,
            const std::vector<Position>* highlightedCells,
            bool blinkPhase,
            int viewportW,
            int viewportH
        ) {
            std::vector<std::string> header = RenderGameHeaderLines(game, message, viewportW);
            std::vector<std::string> board = RenderBoardLines(game, cursor, highlightedCells, blinkPhase);

            std::vector<std::string> panel;
            int panelW = std::min(48, std::max(28, viewportW / 3));
            if (panelView) panel = RenderMenuPanelLines(*panelView, panelW);

            // Vertical nếu đủ cao
            int needH = (int)header.size() + 1 + (int)board.size();
            if (panelView) needH += 1 + (int)panel.size();

            std::vector<std::string> out;
            if (!panelView || needH <= viewportH) {
                out.insert(out.end(), header.begin(), header.end());
                out.push_back(std::string(viewportW, ' '));
                out.insert(out.end(), board.begin(), board.end());
                if (panelView) {
                    out.push_back(std::string(viewportW, ' '));
                    // panel full width trong vertical: re-render theo viewportW
                    std::vector<std::string> panelFull = RenderMenuPanelLines(*panelView, viewportW);
                    out.insert(out.end(), panelFull.begin(), panelFull.end());
                }
                return out;
            }

            // Side-by-side: board trái, panel phải dưới header
            int gap = 2;
            int leftW = viewportW - panelW - gap;
            if (leftW < 20) {
                // Nếu quá hẹp, fallback: không có panel (đỡ bị cắt)
                out.insert(out.end(), header.begin(), header.end());
                out.push_back(std::string(viewportW, ' '));
                for (size_t i = 0; i < board.size(); ++i) out.push_back(PadRight(board[i], viewportW));
                return out;
            }

            // Normalize board lines to leftW
            for (size_t i = 0; i < board.size(); ++i) {
                if ((int)board[i].size() > leftW) board[i] = board[i].substr(0, leftW);
                else board[i] = PadRight(board[i], leftW);
            }
            // Normalize panel lines to panelW
            for (size_t i = 0; i < panel.size(); ++i) panel[i] = PadRight(panel[i], panelW);

            out.insert(out.end(), header.begin(), header.end());
            out.push_back(std::string(viewportW, ' '));

            int rows = std::max((int)board.size(), (int)panel.size());
            for (int i = 0; i < rows; ++i) {
                std::string left = (i < (int)board.size()) ? board[(size_t)i] : std::string(leftW, ' ');
                std::string right = (i < (int)panel.size()) ? panel[(size_t)i] : std::string(panelW, ' ');
                out.push_back(left + std::string(gap, ' ') + right);
            }

            return out;
        }

    } // namespace

    void InitializeConsoleUI() {
        // Giữ tương thích với codebase hiện tại
        SetConsoleOutputCP(65001);
        SetConsoleCP(65001);

        ApplyConsoleInputMode();
        SetCursorVisible(false);
        ClearScreenFast();
    }

    void ClearScreenFast() {
        CONSOLE_SCREEN_BUFFER_INFO csbi = GetCSBI();
        DWORD cellCount = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y;
        DWORD written = 0;
        COORD home{ 0, 0 };

        FillConsoleOutputCharacterA(HOut(), ' ', cellCount, home, &written);
        FillConsoleOutputAttribute(HOut(), csbi.wAttributes, cellCount, home, &written);
        SetConsoleCursorPosition(HOut(), home);
    }

    void SleepMs(int ms) {
        if (ms > 0) Sleep((DWORD)ms);
    }

    void DrawMenuViewEnhanced(const MenuView& view) {
        CONSOLE_SCREEN_BUFFER_INFO csbi = GetCSBI();
        int w = ViewWidth(csbi);
        int h = ViewHeight(csbi);
        if (w <= 0 || h <= 0) return;

        // Menu full-screen panel theo viewportW
        std::vector<std::string> lines = RenderMenuPanelLines(view, w);
        NormalizeToViewport(lines, w, h);
        PresentLines(lines);
    }

    void DrawGameScreenEnhanced(
        const GameSession& game,
        const Position& cursor,
        const std::string& message,
        const std::vector<Position>* highlightedCells,
        bool blinkPhase
    ) {
        CONSOLE_SCREEN_BUFFER_INFO csbi = GetCSBI();
        int w = ViewWidth(csbi);
        int h = ViewHeight(csbi);
        if (w <= 0 || h <= 0) return;

        std::vector<std::string> composed = ComposeGameWithOptionalPanel(
            game, cursor, message,
            /*panel*/ nullptr,
            highlightedCells, blinkPhase, w, h
        );
        NormalizeToViewport(composed, w, h);
        PresentLines(composed);
    }

    void DrawGameMenuOverlayEnhanced(
        const GameSession& game,
        const Position& cursor,
        const std::string& message,
        const MenuView& overlayView,
        const std::vector<Position>* highlightedCells,
        bool blinkPhase
    ) {
        CONSOLE_SCREEN_BUFFER_INFO csbi = GetCSBI();
        int w = ViewWidth(csbi);
        int h = ViewHeight(csbi);
        if (w <= 0 || h <= 0) return;

        std::vector<std::string> composed = ComposeGameWithOptionalPanel(
            game, cursor, message,
            &overlayView,
            highlightedCells, blinkPhase, w, h
        );
        NormalizeToViewport(composed, w, h);
        PresentLines(composed);
    }

    std::vector<Position> FindWinningLineCells(const GameSession& game) {
        std::vector<Position> line;
        if (game.result != GameResult::XWin && game.result != GameResult::OWin) return line;

        CellState winner = (game.result == GameResult::XWin) ? CellState::X : CellState::O;
        int n = GetBoardSize(game);

        const int dirs[4][2] = { {0,1},{1,0},{1,1},{1,-1} };

        auto build = [&](Position start, int dr, int dc) -> bool {
            line.clear();
            int r = start.row, c = start.col;

            while (r >= 0 && r < n && c >= 0 && c < n && GetCell(game, Position(r, c)) == winner) {
                line.push_back(Position(r, c));
                r += dr; c += dc;
            }
            return (int)line.size() >= config::WIN_LENGTH;
            };

        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                if (GetCell(game, Position(r, c)) != winner) continue;

                for (int i = 0; i < 4; ++i) {
                    int pr = r - dirs[i][0];
                    int pc = c - dirs[i][1];
                    if (pr >= 0 && pr < n && pc >= 0 && pc < n &&
                        GetCell(game, Position(pr, pc)) == winner) {
                        continue; // không phải đầu chuỗi
                    }

                    if (build(Position(r, c), dirs[i][0], dirs[i][1])) {
                        if ((int)line.size() > config::WIN_LENGTH) line.resize(config::WIN_LENGTH);
                        return line;
                    }
                }
            }
        }

        line.clear();
        return line;
    }

} // namespace caro
/