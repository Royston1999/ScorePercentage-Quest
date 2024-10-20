#pragma once

#include "HMUI/ModalView.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "Utils/ScoreUtils.hpp"
#include "PPCalculator.hpp"
#include "GlobalNamespace/ScoreFormatter.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/BeatmapKey.hpp"
#include "bsml/shared/BSML/Components/ModalView.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/UI/Button.hpp"
#include "UnityEngine/UI/VerticalLayoutGroup.hpp"

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
            UnityEngine::UI::Button* closeButton;
            UnityEngine::UI::VerticalLayoutGroup* list;
            UnityEngine::GameObject* loadingCircle;
            GlobalNamespace::BeatmapKey* currentMap;
            GlobalNamespace::PlayerData* playerData;
            void updateInfo(std::string text = "");
            void setDisplayTexts(std::string text);
            std::function<void()> onScoreDetails;
            bool hasValidScoreData;
            bool modalSettingsChanged;
    };
    void initModalPopup(ModalPopup** modalUI, UnityEngine::Transform* parent);
}

namespace Sprites{
    static std::string scoreDetailsButton = "iVBORw0KGgoAAAANSUhEUgAAAMgAAADICAYAAACtWK6eAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAB3RJTUUH5gIVFTgbltmyfgAAAAd0RVh0QXV0aG9yAKmuzEgAAAAMdEVYdERlc2NyaXB0aW9uABMJISMAAAAKdEVYdENvcHlyaWdodACsD8w6AAAADnRFWHRDcmVhdGlvbiB0aW1lADX3DwkAAAAJdEVYdFNvZnR3YXJlAF1w/zoAAAALdEVYdERpc2NsYWltZXIAt8C0jwAAAAh0RVh0V2FybmluZwDAG+aHAAAAB3RFWHRTb3VyY2UA9f+D6wAAAAh0RVh0Q29tbWVudAD2zJa/AAAABnRFWHRUaXRsZQCo7tInAAADs0lEQVR4nO3dYXKcVhCF0deUt+LsAfZfsAhnKxH5EY/tyMp15JqBHnTOAqBfwSdgKBW17/sA3jadPQB0JhAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFA8OnsAbiPqqp1XV+O2NeyLNP+Qf4VtT7IOi9t27ZTDuI8z3XGfo/kFuvJnRXH2fs+ikCeWIcTtMMMjySQJ1VVbW5vOs1yb55BnlS3v9xXfR5xBYFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEn84e4ChVVeu6vhyxr2VZpn3f9yP2xWPVRziO27adssh5nutR2z5rTf/lkWs90+Vvsc48kbqdxLzfpQPpcIJ2mIHfd9lAqqrNJb/TLLzPZQM56oH8/+g0C+9z2UDgHgQCwYd5D8JzqqppXde/jtjXW++vXEFoa9u2/ag4xvjnWfH1r44CoaUu768EQjsd3h3dZhAIrVRVm3OyOg0DY4xx5DPHr6zr+iIQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBD89I3CqqqjPlv81jfhoJN/XUG+fhPusG96v/VNOOjkWyBdvgkHnUxj9DhBO8wAr01VVWcPcdNpFhhjjOnIZ45f6TQLjOFnXogEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCASCaVmWNpHcc5arruuR2/xdVz5u077v+9mDjDHGsix/3HOWq67rptH6Pl/5uE1jjDHPc5090L7vX+69zauu66bJ+v689zabrOvLGGPUj8Fu23Z4vV9LfdhJNMZ113Vz0vo+PyKOH3U4bvX6ilZVta7ry0HDHHaLd9V13Vx1fWev66dAgO/a/GIAHQkEAoFAIBAIBAKBQCAQCAQCgUAgEAgEAoFAIBAIBAKBQCAQCAQCgUAgEPwNx3QlGwtr/5QAAAAASUVORK5CYII=";
    }