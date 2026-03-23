#pragma once

#include "MenuTypes.h"

#include <string>
#include <vector>

namespace caro {

    std::string BuildSaveSlotFilePath(
        int slotIndex,
        const std::string& directoryPath = config::DEFAULT_SAVE_DIRECTORY
    );

    std::vector<SaveSlotInfo> BuildSaveSlotInfoList(
        const std::string& directoryPath = config::DEFAULT_SAVE_DIRECTORY
    );

} // namespace caro