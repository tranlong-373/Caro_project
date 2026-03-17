#include "SaveMenuController.h"

#include "MenuNavigation.h"
#include "MenuText.h"

namespace caro {

    namespace {

        int GetSaveMenuItemCount(const MenuContext& context) {
            return static_cast<int>(context.saveSlots.size()) + 1;
        }

        MenuItemView MakeSlotItem(
            const SaveSlotInfo& slot,
            bool selected,
            const std::string& saveNameDraft,
            Language language
        ) {
            MenuItemView item;
            item.id = "save_slot_" + std::to_string(slot.slotIndex);
            item.label = FormatSaveSlotLine(slot, language);
            item.value = saveNameDraft;
            item.kind = MenuItemKind::SaveSlot;
            item.selected = selected;
            item.hint = SelectText(language, "Enter: luu vao slot nay", "Enter: save to this slot");
            return item;
        }

        MenuItemView MakeBackItem(bool selected, Language language) {
            MenuItemView item;
            item.id = "back";
            item.label = SelectText(language, "Quay lai", "Back");
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            return item;
        }

        void GoBack(MenuContext& context) {
            MenuScreen target = context.saveReturnScreen;
            if (target == MenuScreen::SaveGame) {
                target = MenuScreen::Main;
            }
            context.currentScreen = target;
        }

    } // namespace

    MenuView BuildSaveMenuView(const MenuContext& context) {
        const Language lang = context.appSettings.language;

        MenuView view;
        view.screen = MenuScreen::SaveGame;
        view.title = SelectText(lang, "LUU GAME", "SAVE GAME");
        view.subtitle = SelectText(lang, "Ten save hien tai: ", "Current save name: ") + context.saveNameDraft;
        view.message = context.statusMessage;
        view.footerHint = SelectText(lang, "W/S: di chuyen | Enter: luu | ESC: quay lai", "W/S: move | Enter: save | ESC: back");

        for (std::size_t i = 0; i < context.saveSlots.size(); ++i) {
            view.items.push_back(MakeSlotItem(
                context.saveSlots[i],
                context.saveSelected == static_cast<int>(i),
                context.saveNameDraft,
                lang
            ));
        }

        view.items.push_back(MakeBackItem(
            context.saveSelected == static_cast<int>(context.saveSlots.size()),
            lang
        ));

        return view;
    }

    MenuCommand HandleSaveMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;
        const int itemCount = GetSaveMenuItemCount(context);

        if (input == MenuInput::Up) {
            context.saveSelected = WrapSelectionIndex(context.saveSelected, -1, itemCount);
            return command;
        }

        if (input == MenuInput::Down) {
            context.saveSelected = WrapSelectionIndex(context.saveSelected, 1, itemCount);
            return command;
        }

        if (input == MenuInput::Back) {
            GoBack(context);
            return command;
        }

        if (input != MenuInput::Confirm) {
            return command;
        }

        if (context.saveSelected == static_cast<int>(context.saveSlots.size())) {
            GoBack(context);
            return command;
        }

        if (context.saveSelected < 0 || context.saveSelected >= static_cast<int>(context.saveSlots.size())) {
            return command;
        }

        command.type = MenuCommandType::RequestSaveSlot;
        command.slotIndex = context.saveSlots[context.saveSelected].slotIndex;
        command.text = context.saveNameDraft;
        return command;
    }

} // namespace caro