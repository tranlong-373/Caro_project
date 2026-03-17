#include "SettingsMenuController.h"

#include "MenuNavigation.h"
#include "MenuText.h"

namespace caro {

    namespace {

        const int kSettingsItemCount = 7;

        MenuItemView MakeToggleItem(const std::string& id, const std::string& label, bool value, bool selected) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.value = FormatOnOff(value);
            item.kind = MenuItemKind::Toggle;
            item.selected = selected;
            return item;
        }

        MenuItemView MakeSliderItem(const std::string& id, const std::string& label, int value, bool selected) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.value = FormatVolume(value);
            item.kind = MenuItemKind::Slider;
            item.selected = selected;
            return item;
        }

        MenuItemView MakeChoiceItem(const std::string& id, const std::string& label, const std::string& value, bool selected) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.value = value;
            item.kind = MenuItemKind::Choice;
            item.selected = selected;
            return item;
        }

        MenuItemView MakeActionItem(const std::string& id, const std::string& label, bool selected) {
            MenuItemView item;
            item.id = id;
            item.label = label;
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            return item;
        }

        void ToggleSound(GameSettings& settings) {
            settings.soundEnabled = !settings.soundEnabled;
        }

        void ToggleMusic(GameSettings& settings) {
            settings.musicEnabled = !settings.musicEnabled;
        }

        void AdjustSoundVolume(GameSettings& settings, int delta) {
            settings.soundVolume = ClampValue(settings.soundVolume + delta, 0, 100);
        }

        void AdjustMusicVolume(GameSettings& settings, int delta) {
            settings.musicVolume = ClampValue(settings.musicVolume + delta, 0, 100);
        }

        void CycleLanguage(GameSettings& settings) {
            settings.language = (settings.language == Language::Vietnamese)
                ? Language::English
                : Language::Vietnamese;
        }

        void ResetDefault(GameSettings& settings) {
            settings = CreateDefaultSettings();
        }

        void OpenBackDestination(MenuContext& context) {
            context.currentScreen = context.settingsReturnScreen;
        }

    } // namespace

    MenuView BuildSettingsMenuView(const MenuContext& context) {
        MenuView view;
        view.screen = MenuScreen::Settings;
        view.title = "SETTINGS";
        view.subtitle = BuildSettingsSummary(context.appSettings);
        view.message = context.statusMessage;
        view.footerHint = "W/S: move | A/D: change value | Enter: confirm | ESC: back";

        view.items.push_back(MakeToggleItem("sound_enabled", "Sound", context.appSettings.soundEnabled, context.settingsSelected == 0));
        view.items.push_back(MakeToggleItem("music_enabled", "Music", context.appSettings.musicEnabled, context.settingsSelected == 1));
        view.items.push_back(MakeSliderItem("sound_volume", "Sound Volume", context.appSettings.soundVolume, context.settingsSelected == 2));
        view.items.push_back(MakeSliderItem("music_volume", "Music Volume", context.appSettings.musicVolume, context.settingsSelected == 3));
        view.items.push_back(MakeChoiceItem("language", "Language", GetDisplayText(context.appSettings.language), context.settingsSelected == 4));
        view.items.push_back(MakeActionItem("reset_default", "Reset Default", context.settingsSelected == 5));
        view.items.push_back(MakeActionItem("back", "Back", context.settingsSelected == 6));

        return view;
    }

    MenuCommand HandleSettingsMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;

        if (input == MenuInput::Up) {
            context.settingsSelected = WrapSelectionIndex(context.settingsSelected, -1, kSettingsItemCount);
            return command;
        }

        if (input == MenuInput::Down) {
            context.settingsSelected = WrapSelectionIndex(context.settingsSelected, 1, kSettingsItemCount);
            return command;
        }

        if (input == MenuInput::Left) {
            if (context.settingsSelected == 0) ToggleSound(context.appSettings);
            else if (context.settingsSelected == 1) ToggleMusic(context.appSettings);
            else if (context.settingsSelected == 2) AdjustSoundVolume(context.appSettings, -10);
            else if (context.settingsSelected == 3) AdjustMusicVolume(context.appSettings, -10);
            else if (context.settingsSelected == 4) CycleLanguage(context.appSettings);
            else return command;

            command.type = MenuCommandType::ApplySettings;
            command.settings = context.appSettings;
            return command;
        }

        if (input == MenuInput::Right) {
            if (context.settingsSelected == 0) ToggleSound(context.appSettings);
            else if (context.settingsSelected == 1) ToggleMusic(context.appSettings);
            else if (context.settingsSelected == 2) AdjustSoundVolume(context.appSettings, 10);
            else if (context.settingsSelected == 3) AdjustMusicVolume(context.appSettings, 10);
            else if (context.settingsSelected == 4) CycleLanguage(context.appSettings);
            else return command;

            command.type = MenuCommandType::ApplySettings;
            command.settings = context.appSettings;
            return command;
        }

        if (input == MenuInput::Back) {
            OpenBackDestination(context);
            return command;
        }

        if (input != MenuInput::Confirm) {
            return command;
        }

        if (context.settingsSelected == 0) {
            ToggleSound(context.appSettings);
            command.type = MenuCommandType::ApplySettings;
            command.settings = context.appSettings;
        }
        else if (context.settingsSelected == 1) {
            ToggleMusic(context.appSettings);
            command.type = MenuCommandType::ApplySettings;
            command.settings = context.appSettings;
        }
        else if (context.settingsSelected == 4) {
            CycleLanguage(context.appSettings);
            command.type = MenuCommandType::ApplySettings;
            command.settings = context.appSettings;
        }
        else if (context.settingsSelected == 5) {
            ResetDefault(context.appSettings);
            command.type = MenuCommandType::ApplySettings;
            command.settings = context.appSettings;
        }
        else if (context.settingsSelected == 6) {
            OpenBackDestination(context);
        }

        return command;
    }

} // namespace caro