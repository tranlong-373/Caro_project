#pragma once

namespace caro {

    int WrapSelectionIndex(int currentIndex, int delta, int itemCount);
    int ClampValue(int value, int minValue, int maxValue);

} // namespace caro