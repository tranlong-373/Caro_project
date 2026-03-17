#include "SaveMenuController.h"

#include "MenuNavigation.h"
#include "MenuText.h"

namespace caro {

    namespace {

        int GetSaveMenuItemCount(const MenuContext& context) {
            return static_cast<int>(context.saveSlots.size()) + 1;
        }

        MenuItemView MakeSlotItem(const SaveSlotInfo& slot, bool selected, const std::string& saveNameDraft) {
            MenuItemView item;
            item.id = "save_slot_" + std::to_string(slot.slotIndex);
            item.label = FormatSaveSlotLine(slot);
            item.value = saveNameDraft;
            item.kind = MenuItemKind::SaveSlot;
            item.selected = selected;
            item.hint = "Enter: save to this slot";
            return item;
        }

        MenuItemView MakeBackItem(bool selected) {
            MenuItemView item;
            item.id = "back";
            item.label = "Back";
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            return item;
        }

        void GoBack(MenuContext& context) {
            context.currentScreen = context.saveReturnScreen;
        }

    } // namespace

    MenuView BuildSaveMenuView(const MenuContext& context) {
        MenuView view;
        view.screen = MenuScreen::SaveGame;
        view.title = "SAVE GAME";
        view.subtitle = "Current save name draft: " + context.saveNameDraft;
        view.message = context.statusMessage;
        view.footerHint = "W/S: move | Enter: save | ESC: back";

        for (std::size_t i = 0; i < context.saveSlots.size(); ++i) {
            view.items.push_back(MakeSlotItem(
                context.saveSlots[i],
                context.saveSelected == static_cast<int>(i),
                context.saveNameDraft
            ));
        }

        view.items.push_back(MakeBackItem(context.saveSelected == static_cast<int>(context.saveSlots.size())));
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