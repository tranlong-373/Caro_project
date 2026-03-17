#pragma once

#include "MenuTypes.h"

namespace caro {

    std::string SelectText(
        Language language,
        const std::string& vietnamese,
        const std::string& english
    );

    std::string GetDisplayText(GameMode value);
    std::string GetDisplayText(GameMode value, Language language);

    std::string GetDisplayText(RuleMode value);
    std::string GetDisplayText(RuleMode value, Language language);

    std::string GetDisplayText(AIDifficulty value);
    std::string GetDisplayText(AIDifficulty value, Language language);

    std::string GetDisplayText(Language value);
    std::string GetDisplayText(Language value, Language uiLanguage);

    std::string GetDisplayText(GameResult value);
    std::string GetDisplayText(GameResult value, Language language);

    std::string FormatOnOff(bool value);
    std::string FormatOnOff(bool value, Language language);

    std::string FormatVolume(int value);

    std::string FormatSaveSlotLine(const SaveSlotInfo& slot);
    std::string FormatSaveSlotLine(const SaveSlotInfo& slot, Language language);

    std::string BuildSettingsSummary(const GameSettings& settings);
    std::string BuildSettingsSummary(const GameSettings& settings, Language language);

    std::string GetBotDisplayName(AIDifficulty difficulty, Language language);

} // namespace caro