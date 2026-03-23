#include "LoadMenuController.h"

#include "MenuNavigation.h"
#include "MenuText.h"

namespace caro {

    namespace {

        int GetLoadMenuItemCount(const MenuContext& context) {
            return static_cast<int>(context.saveSlots.size()) + 1;
        }

        MenuItemView MakeSlotItem(const SaveSlotInfo& slot, bool selected) {
            MenuItemView item;
            item.id = "slot_" + std::to_string(slot.slotIndex);
            item.label = FormatSaveSlotLine(slot);
            item.kind = MenuItemKind::SaveSlot;
            item.selected = selected;
            item.enabled = true;
            item.hint = "Enter: Load | A: Rename | D: Delete";
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

        int GetSelectedSlotIndex(const MenuContext& context) {
            if (context.loadSelected < 0 || context.loadSelected >= static_cast<int>(context.saveSlots.size())) {
                return -1;
            }
            return context.saveSlots[context.loadSelected].slotIndex;
        }

        bool IsSelectedSlotOccupied(const MenuContext& context) {
            if (context.loadSelected < 0 || context.loadSelected >= static_cast<int>(context.saveSlots.size())) {
                return false;
            }
            return context.saveSlots[context.loadSelected].occupied;
        }

    } // namespace

    MenuView BuildLoadMenuView(const MenuContext& context) {
        MenuView view;
        view.screen = MenuScreen::LoadGame;
        view.title = "LOAD GAME";
        view.subtitle = "Format: Name | Date | Time | Mode | Rule";
        view.message = context.statusMessage;
        view.footerHint = "W/S: move | Enter: load | A: rename | D: delete | ESC: back";

        for (std::size_t i = 0; i < context.saveSlots.size(); ++i) {
            view.items.push_back(MakeSlotItem(context.saveSlots[i], context.loadSelected == static_cast<int>(i)));
        }

        view.items.push_back(MakeBackItem(context.loadSelected == static_cast<int>(context.saveSlots.size())));
        return view;
    }

    MenuCommand HandleLoadMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;
        const int itemCount = GetLoadMenuItemCount(context);

        if (input == MenuInput::Up) {
            context.loadSelected = WrapSelectionIndex(context.loadSelected, -1, itemCount);
            return command;
        }

        if (input == MenuInput::Down) {
            context.loadSelected = WrapSelectionIndex(context.loadSelected, 1, itemCount);
            return command;
        }

        if (input == MenuInput::Back) {
            OpenMainMenuScreen(context);
            return command;
        }

        const bool onBackItem = (context.loadSelected == static_cast<int>(context.saveSlots.size()));
        if (onBackItem) {
            if (input == MenuInput::Confirm) {
                OpenMainMenuScreen(context);
            }
            return command;
        }

        if (!IsSelectedSlotOccupied(context)) {
            if (input == MenuInput::Confirm || input == MenuInput::Left || input == MenuInput::Right) {
                SetMenuStatusMessage(context, "Selected slot is empty.");
            }
            return command;
        }

        command.slotIndex = GetSelectedSlotIndex(context);

        if (input == MenuInput::Confirm) {
            command.type = MenuCommandType::RequestLoadSlot;
        }
        else if (input == MenuInput::Left) {
            command.type = MenuCommandType::RequestRenameSlot;
            command.text = context.saveSlots[context.loadSelected].metadata.saveName;
        }
        else if (input == MenuInput::Right) {
            command.type = MenuCommandType::RequestDeleteSlot;
        }

        return command;
    }

} // namespace caro