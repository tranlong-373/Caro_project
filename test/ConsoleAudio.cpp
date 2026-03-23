#include "ConsoleAudio.h"

#include <windows.h>
#include <thread>
#include <atomic>

namespace caro {

    namespace {
        std::atomic<bool> g_soundBusy(false);

        int ClampVolume(int volume) {
            if (volume < 0) return 0;
            if (volume > 100) return 100;
            return volume;
        }

        int ScaleDuration(int baseDurationMs, int volume) {
            int safeVolume = ClampVolume(volume);
            int reduced = baseDurationMs - safeVolume / 8;
            if (reduced < 8) reduced = 8;
            return reduced;
        }

        void ToneAsync(bool enabled, int volume, int frequency, int durationMs) {
            const int safeVolume = ClampVolume(volume);

            if (!enabled || safeVolume <= 0) {
                return;
            }

            if (g_soundBusy.exchange(true)) {
                return;
            }

            if (frequency < 37) frequency = 37;
            if (frequency > 32767) frequency = 32767;

            const int finalDuration = ScaleDuration(durationMs, safeVolume);

            std::thread([frequency, finalDuration]() {
                Beep(frequency, finalDuration);
                g_soundBusy = false;
                }).detach();
        }
    }

    void PlayMenuMoveSound(const GameSettings& settings) {
        ToneAsync(settings.soundEnabled, settings.soundVolume, 700, 10);
    }

    void PlayConfirmSound(const GameSettings& settings) {
        ToneAsync(settings.soundEnabled, settings.soundVolume, 900, 12);
    }

    void PlayPlaceSound(const GameSettings& settings, CellState symbol) {
        if (symbol == CellState::X) {
            ToneAsync(settings.soundEnabled, settings.soundVolume, 950, 12);
        }
        else if (symbol == CellState::O) {
            ToneAsync(settings.soundEnabled, settings.soundVolume, 650, 12);
        }
        else {
            ToneAsync(settings.soundEnabled, settings.soundVolume, 800, 10);
        }
    }

    void PlayInvalidSound(const GameSettings& settings) {
        ToneAsync(settings.soundEnabled, settings.soundVolume, 260, 14);
    }

    void PlayStartGameSound(const GameSettings& settings) {
        ToneAsync(settings.soundEnabled, settings.soundVolume, 880, 14);
    }

    void PlayResultSound(const GameSettings& settings, GameResult result) {
        if (!settings.soundEnabled || settings.soundVolume <= 0) {//
            return;
        }

        if (result == GameResult::Draw) {
            ToneAsync(true, settings.soundVolume, 700, 14);
            return;
        }

        if (result == GameResult::XWin || result == GameResult::OWin) {
            ToneAsync(true, settings.soundVolume, 1100, 16);
        }
    }

} // namespace caro/