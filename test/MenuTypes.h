#pragma once

#include "Types.h"

#include <string>
#include <vector>

namespace caro {

    enum class MenuScreen {
        Main = 0,
        NewGame = 1,
        Settings = 2,
        LoadGame = 3,
        SaveGame = 4,
        Pause = 5,
        Result = 6,
        HowToPlay = 7,
        AboutUs = 8
    };

    enum class MenuInput {
        None = 0,
        Up = 1,
        Down = 2,
        Left = 3,
        Right = 4,
        Confirm = 5,
        Back = 6
    };

    enum class MenuItemKind {
        Action = 0,
        Toggle = 1,
        Choice = 2,
        Slider = 3,
        Text = 4,
        SaveSlot = 5
    };

    enum class MenuCommandType {
        None = 0,
        StartNewGame = 1,
        RequestLoadSlot = 2,
        RequestSaveSlot = 3,
        RequestRenameSlot = 4,
        RequestDeleteSlot = 5,
        ApplySettings = 6,
        ContinueGame = 7,
        RestartGame = 8,
        BackToMainMenu = 9,
        OpenPauseMenu = 10,
        OpenLoadMenu = 11,
        OpenSaveMenu = 12,
        OpenSettings = 13,
        OpenHowToPlay = 14,
        OpenAboutUs = 15,
        ExitApplication = 16
    };

    struct SaveSlotInfo {
        int slotIndex;
        bool occupied;
        std::string filePath;
        SaveMetadata metadata;

        SaveSlotInfo()
            : slotIndex(0),
            occupied(false),
            filePath(""),
            metadata() {
        }
    };

    struct MenuItemView {
        std::string id;
        std::string label;
        std::string value;
        std::string hint;
        MenuItemKind kind;
        bool selected;
        bool enabled;

        MenuItemView()
            : id(""),
            label(""),
            value(""),
            hint(""),
            kind(MenuItemKind::Action),
            selected(false),
            enabled(true) {
        }
    };

    struct MenuView {
        MenuScreen screen;
        std::string title;
        std::string subtitle;
        std::string message;
        std::string footerHint;
        std::vector<MenuItemView> items;

        MenuView() : screen(MenuScreen::Main) {
        }
    };

    struct NewGameDraft {
        GameSettings settings;
        std::string playerXName;
        std::string playerOName;

        NewGameDraft()
            : settings(),
            playerXName(config::DEFAULT_PLAYER_X_NAME),
            playerOName(config::DEFAULT_PLAYER_O_NAME) {
        }
    };

    struct MenuCommand {
        MenuCommandType type;
        int slotIndex;
        GameSettings settings;
        std::string text;
        std::string extraText;

        MenuCommand()
            : type(MenuCommandType::None),
            slotIndex(0),
            settings(),
            text(""),
            extraText("") {
        }
    };

} // namespace caro