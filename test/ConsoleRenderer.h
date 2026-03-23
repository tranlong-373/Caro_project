#pragma once

#include "MenuTypes.h"
#include "Types.h"

#include <string>
#include <vector>

namespace caro {

    void InitializeConsoleUI();
    void ClearScreenFast();
    void SleepMs(int ms);

    void DrawMenuViewEnhanced(const MenuView& view);

    void DrawGameScreenEnhanced(
        const GameSession& game,
        const Position& cursor,
        const std::string& message,
        const std::vector<Position>* highlightedCells = 0,
        bool blinkPhase = false
    );

    // New (không đổi logic, chỉ để vẽ overlay menu lên nền bàn cờ)
    void DrawGameMenuOverlayEnhanced(
        const GameSession& game,
        const Position& cursor,
        const std::string& message,
        const MenuView& overlayView,
        const std::vector<Position>* highlightedCells = 0,
        bool blinkPhase = false
    );

    std::vector<Position> FindWinningLineCells(const GameSession& game);

} // namespace caro
