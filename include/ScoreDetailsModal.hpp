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

DECLARE_CLASS_CODEGEN(ScorePercentage, ModalPopup, HMUI::ModalView,
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, title); 
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, score);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, maxCombo);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, playCount);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, missCount);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, badCutCount);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, pauseCountGUI);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, datePlayed);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::Button*, openButton);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::GridLayoutGroup*, buttonHolder);
    DECLARE_INSTANCE_FIELD(UnityEngine::UI::VerticalLayoutGroup*, list);

    DECLARE_INSTANCE_METHOD(void, updateInfo, GlobalNamespace::PlayerLevelStatsData* playerLevelStatsData, GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap);
    DECLARE_INSTANCE_METHOD(void, initModal, int uiText);

    public:
        static ScorePercentage::ModalPopup* CreatePopupModal(UnityEngine::Transform* parent);
)