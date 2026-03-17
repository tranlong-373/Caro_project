#include "MenuText.h"

#include <sstream>

namespace caro {

    std::string SelectText(
        Language language,
        const std::string& vietnamese,
        const std::string& english
    ) {
        return (language == Language::Vietnamese) ? vietnamese : english;
    }

    std::string GetDisplayText(GameMode value) {
        return GetDisplayText(value, Language::English);
    }

    std::string GetDisplayText(GameMode value, Language language) {
        switch (value) {
        case GameMode::PVP:
            return "PVP";
        case GameMode::PVE:
            return "PVE";
        default:
            return SelectText(language, "Khong ro", "Unknown");
        }
    }

    std::string GetDisplayText(RuleMode value) {
        return GetDisplayText(value, Language::English);
    }

    std::string GetDisplayText(RuleMode value, Language language) {
        switch (value) {
        case RuleMode::FreeStyle:
            return SelectText(language, "Tu do", "FreeStyle");
        case RuleMode::Standard:
            return SelectText(language, "Tieu chuan", "Standard");
        default:
            return SelectText(language, "Khong ro", "Unknown");
        }
    }

    std::string GetDisplayText(AIDifficulty value) {
        return GetDisplayText(value, Language::English);
    }

    std::string GetDisplayText(AIDifficulty value, Language language) {
        switch (value) {
        case AIDifficulty::Easy:
            return SelectText(language, "De", "Easy");
        case AIDifficulty::Medium:
            return SelectText(language, "Trung binh", "Medium");
        case AIDifficulty::Hard:
            return SelectText(language, "Kho", "Hard");
        case AIDifficulty::Master:
            return SelectText(language, "Master", "Master");
        default:
            return SelectText(language, "Khong ro", "Unknown");
        }
    }

    std::string GetDisplayText(Language value) {
        return GetDisplayText(value, Language::English);
    }

    std::string GetDisplayText(Language value, Language uiLanguage) {
        switch (value) {
        case Language::Vietnamese:
            return SelectText(uiLanguage, "Tieng Viet", "Vietnamese");
        case Language::English:
            return "English";
        default:
            return SelectText(uiLanguage, "Khong ro", "Unknown");
        }
    }

    std::string GetDisplayText(GameResult value) {
        return GetDisplayText(value, Language::English);
    }

    std::string GetDisplayText(GameResult value, Language language) {
        switch (value) {
        case GameResult::XWin:
            return SelectText(language, "X thang", "X wins");
        case GameResult::OWin:
            return SelectText(language, "O thang", "O wins");
        case GameResult::Draw:
            return SelectText(language, "Hoa", "Draw");
        case GameResult::InProgress:
            return SelectText(language, "Dang choi", "In Progress");
        default:
            return SelectText(language, "Khong ro", "Unknown");
        }
    }

    std::string FormatOnOff(bool value) {
        return value ? "ON" : "OFF";
    }

    std::string FormatOnOff(bool value, Language language) {
        if (language == Language::Vietnamese) {
            return value ? "BAT" : "TAT";
        }
        return value ? "ON" : "OFF";
    }

    std::string FormatVolume(int value) {
        std::ostringstream oss;
        oss << value << "%";
        return oss.str();
    }

    std::string GetBotDisplayName(AIDifficulty difficulty, Language language) {
        std::ostringstream oss;
        oss << "BOT - " << GetDisplayText(difficulty, language);
        return oss.str();
    }

    std::string FormatSaveSlotLine(const SaveSlotInfo& slot) {
        return FormatSaveSlotLine(slot, Language::English);
    }

    std::string FormatSaveSlotLine(const SaveSlotInfo& slot, Language language) {
        std::ostringstream oss;
        oss << "Slot " << slot.slotIndex << " | ";

        if (!slot.occupied) {
            oss << SelectText(language, "[Trong]", "[Empty]");
            return oss.str();
        }

        oss << (slot.metadata.saveName.empty() ? SelectText(language, "Khong ten", "Unnamed") : slot.metadata.saveName);
        oss << " | " << slot.metadata.displayDate;
        oss << " | " << slot.metadata.displayTime;
        oss << " | " << GetDisplayText(slot.metadata.gameMode, language);
        oss << " | " << GetDisplayText(slot.metadata.ruleMode, language);
        return oss.str();
    }

    std::string BuildSettingsSummary(const GameSettings& settings) {
        return BuildSettingsSummary(settings, settings.language);
    }

    std::string BuildSettingsSummary(const GameSettings& settings, Language language) {
        std::ostringstream oss;

        oss
            << SelectText(language, "Am thanh", "Sound") << "=" << FormatOnOff(settings.soundEnabled, language)
            << ", "
            << SelectText(language, "Nhac", "Music") << "=" << FormatOnOff(settings.musicEnabled, language)
            << ", "
            << SelectText(language, "Ngon ngu", "Language") << "=" << GetDisplayText(settings.language, language);

        return oss.str();
    }

} // namespace caro