#pragma once

#include "Types.h"

namespace caro {

    void PlayMenuMoveSound(const GameSettings& settings);
    void PlayConfirmSound(const GameSettings& settings);
    void PlayPlaceSound(const GameSettings& settings, CellState symbol);
    void PlayInvalidSound(const GameSettings& settings);
    void PlayStartGameSound(const GameSettings& settings);
    void PlayResultSound(const GameSettings& settings, GameResult result);

} // namespace caro