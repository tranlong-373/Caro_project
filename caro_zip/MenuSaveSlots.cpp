#include "MenuSaveSlots.h"

#include <fstream>
#include <limits>
#include <sstream>

namespace caro {

    namespace {

        std::string NormalizeSaveDirectory(const std::string& directoryPath) {
            std::string dir = directoryPath.empty()
                ? std::string(config::DEFAULT_SAVE_DIRECTORY)
                : directoryPath;

            while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) {
                dir.pop_back();
            }

            if (dir.empty()) {
                dir = "saves";
            }

            return dir;
        }

        bool FileExists(const std::string& path) {
            std::ifstream fin(path.c_str());
            return fin.good();
        }

        bool ReadMetadataFromSaveFile(const std::string& filePath, SaveMetadata& meta) {
            std::ifstream fin(filePath.c_str());
            if (!fin.is_open()) return false;

            std::string header;
            std::getline(fin, header);
            if (header != "CARO_SAVE_V1") return false;

            int boardSize = 0;
            fin >> boardSize;

            int gameMode = 0;
            int ruleMode = 0;
            int aiDifficulty = 0;
            fin >> gameMode >> ruleMode >> aiDifficulty;

            int soundEnabled = 0;
            int musicEnabled = 0;
            int soundVolume = 0;
            int musicVolume = 0;
            int language = 0;
            fin >> soundEnabled >> musicEnabled >> soundVolume >> musicVolume >> language;

            fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::string playerXName;
            std::string playerOName;
            std::getline(fin, playerXName);
            std::getline(fin, playerOName);

            int currentTurn = 0;
            int result = 0;
            int screen = 0;
            int isPaused = 0;
            fin >> currentTurn >> result >> screen >> isPaused;

            int moveCount = 0;
            int xMoveCount = 0;
            int oMoveCount = 0;
            int xWinCount = 0;
            int oWinCount = 0;
            int drawCount = 0;
            fin >> moveCount >> xMoveCount >> oMoveCount >> xWinCount >> oWinCount >> drawCount;

            fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            std::getline(fin, meta.saveName);
            std::getline(fin, meta.displayDate);
            std::getline(fin, meta.displayTime);

            meta.gameMode = (GameMode)gameMode;
            meta.ruleMode = (RuleMode)ruleMode;

            return true;
        }

    } // namespace

    std::string BuildSaveSlotFilePath(int slotIndex, const std::string& directoryPath) {
        std::ostringstream oss;
        oss << NormalizeSaveDirectory(directoryPath)
            << "/slot"
            << slotIndex
            << config::DEFAULT_SAVE_EXTENSION;
        return oss.str();
    }

    std::vector<SaveSlotInfo> BuildSaveSlotInfoList(const std::string& directoryPath) {
        std::vector<SaveSlotInfo> result;

        for (int i = 1; i <= config::MAX_SAVE_SLOTS; ++i) {
            SaveSlotInfo slot;
            slot.slotIndex = i;
            slot.filePath = BuildSaveSlotFilePath(i, directoryPath);
            slot.occupied = false;

            if (FileExists(slot.filePath)) {
                SaveMetadata meta;
                if (ReadMetadataFromSaveFile(slot.filePath, meta)) {
                    slot.occupied = true;
                    slot.metadata = meta;

                    if (slot.metadata.saveName.empty()) {
                        std::ostringstream oss;
                        oss << "slot" << i;
                        slot.metadata.saveName = oss.str();
                    }
                }
            }

            result.push_back(slot);
        }

        return result;
    }

} // namespace caro