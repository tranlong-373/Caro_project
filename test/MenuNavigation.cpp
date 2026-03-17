#include "MenuNavigation.h"

namespace caro {

    int WrapSelectionIndex(int currentIndex, int delta, int itemCount) {
        if (itemCount <= 0) return 0;

        int next = currentIndex + delta;
        while (next < 0) next += itemCount;
        while (next >= itemCount) next -= itemCount;
        return next;
    }

    int ClampValue(int value, int minValue, int maxValue) {
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }

} // namespace caro