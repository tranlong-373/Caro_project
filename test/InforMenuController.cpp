#include "InforMenuController.h"
#include "MenuText.h"

namespace caro {

    namespace {

        MenuItemView MakeBackItem(bool selected, Language language) {
            MenuItemView item;
            item.id = "back";
            item.label = SelectText(language, "Quay lai", "Back");
            item.kind = MenuItemKind::Action;
            item.selected = selected;
            return item;
        }

        void GoBackFromInfo(MenuContext& context) {
            MenuScreen target = context.infoReturnScreen;
            if (target == MenuScreen::HowToPlay || target == MenuScreen::AboutUs) {
                target = MenuScreen::Main;
            }
            context.currentScreen = target;
        }

    } // namespace

    MenuView BuildHowToPlayView(const MenuContext& context) {
        const Language lang = context.appSettings.language;

        MenuView view;
        view.screen = MenuScreen::HowToPlay;
        view.title = SelectText(lang, "CACH CHOI", "HOW TO PLAY");
        view.subtitle = SelectText(lang, "Dieu khien: W/A/S/D de di chuyen, Enter de xac nhan.", "Controls: W/A/S/D to move, Enter to confirm.");
        view.message = SelectText(
            lang,
            "Huong dan:\n"
            "- W/A/S/D de di chuyen con tro.\n"
            "- Enter de dat quan hoac xac nhan trong menu.\n"
            "- Tu do: co 5 quan lien tiep la thang.\n"
            "- Tieu chuan: 5 quan bi chan 2 dau khong tinh thang.",
            "Guide:\n"
            "- Use W/A/S/D to move the cursor.\n"
            "- Use Enter to place a piece or confirm a menu item.\n"
            "- FreeStyle: any 5 in a row wins.\n"
            "- Standard: 5 in a row blocked at both ends does not count."
        );
        view.footerHint = SelectText(lang, "Enter / ESC: quay lai", "Enter / ESC: back");
        view.items.push_back(MakeBackItem(true, lang));
        return view;
    }

    MenuView BuildAboutUsView(const MenuContext& context) {
        const Language lang = context.appSettings.language;

        MenuView view;
        view.screen = MenuScreen::AboutUs;
        view.title = "ABOUT US";
        view.subtitle = SelectText(lang, "Man hinh nay tach rieng de UI team thiet ke sau.", "This screen is separated so the UI team can design it later.");
        view.message = SelectText(
            lang,
            "Noi dung goi y:\n"
            "- Ten nhom\n"
            "- Thanh vien va vai tro\n"
            "- Nguon assets\n"
            "- Cong nghe su dung: engine, UI, sound",
            "Suggested content:\n"
            "- Team name\n"
            "- Members and roles\n"
            "- Asset credits\n"
            "- Tech stack: engine, UI, sound"
        );
        view.footerHint = SelectText(lang, "Enter / ESC: quay lai", "Enter / ESC: back");
        view.items.push_back(MakeBackItem(true, lang));
        return view;
    }

    MenuCommand HandleInfoMenuInput(MenuContext& context, MenuInput input) {
        MenuCommand command;
        if (input == MenuInput::Back || input == MenuInput::Confirm) {
            GoBackFromInfo(context);
        }
        return command;
    }

} // namespace caro