#include "GameRules.h"
#include "GameCore.h"

namespace caro {

    namespace {

        int CountOneDirection(
            const GameSession& game,
            Position start,
            CellState symbol,
            int dRow,
            int dCol
        ) {
            int count = 0;
            int r = start.row + dRow;
            int c = start.col + dCol;
            int n = (int)game.board.size();

            while (r >= 0 && r < n && c >= 0 && c < n && game.board[r][c] == symbol) {
                ++count;
                r += dRow;
                c += dCol;
            }

            return count;
        }

        bool IsBlockedEnd(
            const GameSession& game,
            int row,
            int col,
            CellState symbol
        ) {
            int n = (int)game.board.size();

            if (row < 0 || row >= n || col < 0 || col >= n) {
                return true;
            }

            CellState value = game.board[row][col];
            return (value != CellState::Empty && value != symbol);
        }

        std::vector<Position> CollectConnectedLine(
            const GameSession& game,
            Position lastMove,
            CellState symbol,
            int dRow,
            int dCol
        ) {
            std::vector<Position> backPart;
            std::vector<Position> line;

            int n = (int)game.board.size();

            int r = lastMove.row - dRow;
            int c = lastMove.col - dCol;
            while (r >= 0 && r < n && c >= 0 && c < n && game.board[r][c] == symbol) {
                backPart.push_back(Position(r, c));
                r -= dRow;
                c -= dCol;
            }

            for (int i = (int)backPart.size() - 1; i >= 0; --i) {
                line.push_back(backPart[i]);
            }

            line.push_back(lastMove);

            r = lastMove.row + dRow;
            c = lastMove.col + dCol;
            while (r >= 0 && r < n && c >= 0 && c < n && game.board[r][c] == symbol) {
                line.push_back(Position(r, c));
                r += dRow;
                c += dCol;
            }

            return line;
        }

        bool IsStandardWinningLine(
            const GameSession& game,
            const std::vector<Position>& line,
            CellState symbol,
            int dRow,
            int dCol
        ) {
            if ((int)line.size() < config::WIN_LENGTH) {
                return false;
            }

            const Position& first = line.front();
            const Position& last = line.back();

            bool blocked1 = IsBlockedEnd(game, first.row - dRow, first.col - dCol, symbol);
            bool blocked2 = IsBlockedEnd(game, last.row + dRow, last.col + dCol, symbol);

            return !(blocked1 && blocked2);
        }

        std::vector<Position> FindWinningLineForRule(
            const GameSession& game,
            Position lastMove,
            CellState symbol,
            RuleMode ruleMode
        ) {
            static const int directions[4][2] = {
                {0, 1}, {1, 0}, {1, 1}, {1, -1}
            };

            if (!IsInsideBoard(game, lastMove)) return std::vector<Position>();
            if (symbol == CellState::Empty) return std::vector<Position>();

            for (int i = 0; i < 4; ++i) {
                int dRow = directions[i][0];
                int dCol = directions[i][1];

                std::vector<Position> line = CollectConnectedLine(game, lastMove, symbol, dRow, dCol);

                if ((int)line.size() < config::WIN_LENGTH) {
                    continue;
                }

                if (ruleMode == RuleMode::FreeStyle) {
                    return line;
                }

                if (IsStandardWinningLine(game, line, symbol, dRow, dCol)) {
                    return line;
                }
            }

            return std::vector<Position>();
        }

        GameResult ResultFromCell(CellState symbol) {
            if (symbol == CellState::X) return GameResult::XWin;
            if (symbol == CellState::O) return GameResult::OWin;
            return GameResult::InProgress;
        }

    } // namespace

    int CountContinuousCells(
        const GameSession& game,
        Position start,
        CellState symbol,
        int dRow,
        int dCol
    ) {
        if (!IsInsideBoard(game, start)) return 0;
        if (GetCell(game, start) != symbol) return 0;

        int total = 1;
        total += CountOneDirection(game, start, symbol, dRow, dCol);
        total += CountOneDirection(game, start, symbol, -dRow, -dCol);
        return total;
    }

    bool HasFiveInRow(
        const GameSession& game,
        Position lastMove,
        CellState symbol
    ) {
        return !FindWinningLineForRule(game, lastMove, symbol, RuleMode::FreeStyle).empty();
    }

    bool IsStandardRuleWinningLine(
        const GameSession& game,
        Position lastMove,
        CellState symbol
    ) {
        return !FindWinningLineForRule(game, lastMove, symbol, RuleMode::Standard).empty();
    }

    std::vector<Position> FindWinningLine(
        const GameSession& game,
        Position lastMove,
        CellState symbol
    ) {
        return FindWinningLineForRule(game, lastMove, symbol, game.settings.ruleMode);
    }

    GameResult EvaluateBoard(const GameSession& game, Position lastMove) {
        if (!IsInsideBoard(game, lastMove)) {
            return GameResult::InProgress;
        }

        CellState symbol = GetCell(game, lastMove);
        if (symbol == CellState::Empty) {
            return IsBoardFull(game) ? GameResult::Draw : GameResult::InProgress;
        }

        std::vector<Position> line = FindWinningLine(game, lastMove, symbol);
        if (!line.empty()) {
            return ResultFromCell(symbol);
        }

        if (IsBoardFull(game)) {
            return GameResult::Draw;
        }

        return GameResult::InProgress;
    }

} // namespace caro