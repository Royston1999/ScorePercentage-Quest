#pragma once

#include "custom-types/shared/macros.hpp"
#include "custom-types/shared/register.hpp"
#include "modloader/shared/modloader.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/QuestUI.hpp"
#include "HMUI/ModalView.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "ScoreUtils.hpp"
#include "PPCalculator.hpp"
#include "GlobalNamespace/ScoreFormatter.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "main.hpp"

namespace ScorePercentage{
    class ModalPopup{
        public:
            HMUI::ModalView* modal;
            TMPro::TextMeshProUGUI* title;
            TMPro::TextMeshProUGUI* score;
            TMPro::TextMeshProUGUI* maxCombo;
            TMPro::TextMeshProUGUI* playCount;
            TMPro::TextMeshProUGUI* missCount;
            TMPro::TextMeshProUGUI* badCutCount;
            TMPro::TextMeshProUGUI* pauseCountGUI;
            TMPro::TextMeshProUGUI* datePlayed;
            UnityEngine::UI::Button* openButton;
            UnityEngine::UI::VerticalLayoutGroup* list;
            void updateInfo();
    };
    void initModalPopup(ModalPopup** modalUI, UnityEngine::Transform* parent);
}