#include "UI/ScoreDetailsUIViewController.hpp"
#include "ScorePercentageConfig.hpp"
#include "Utils/UIUtils.hpp"
#include "main.hpp"

DEFINE_TYPE(ScoreDetailsUI::Views, ScoreDetailsUIViewController);
BSML::ToggleSetting* ppToggle = nullptr;
BSML::ToggleSetting* playCountToggle = nullptr;
BSML::ToggleSetting* missCountToggle = nullptr;
BSML::ToggleSetting* badCutCountToggle = nullptr;
BSML::ToggleSetting* pauseCountToggle = nullptr;
BSML::ToggleSetting* datePlayedToggle = nullptr;
BSML::ToggleSetting* alwaysOnToggle = nullptr;
UnityEngine::UI::VerticalLayoutGroup* scoreDetailsContainer;
ArrayW<TMPro::TextMeshProUGUI*> toggleText;

void toggleToggles(bool value){
    toggleText = scoreDetailsContainer->get_transform()->GetComponentsInChildren<TMPro::TextMeshProUGUI*>();
    ppToggle->set_interactable(value);
    playCountToggle->set_interactable(value);
    missCountToggle->set_interactable(value);
    badCutCountToggle->set_interactable(value);
    pauseCountToggle->set_interactable(value);
    datePlayedToggle->set_interactable(value);
    alwaysOnToggle->set_interactable(value);
    auto colour = value ? UnityEngine::Color::get_white() : UnityEngine::Color::get_gray();
    for(int i=0; i<toggleText.size(); i++) 
        if ((i!=0 && i%3==0) || (i!=1 && i%3==2 && toggleText[i]->get_alpha()>0))
            toggleText[i]->set_color(colour);
}

void ScoreDetailsUI::Views::ScoreDetailsUIViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling){
    using namespace GlobalNamespace;
    using namespace UnityEngine;
    using namespace BSML::Lite;
    using namespace UnityEngine::UI;

    if (firstActivation) {
        scoreDetailsContainer = CreateVerticalLayoutGroup(get_transform());
        UIUtils::AddHeader(get_transform(), "Score Details UI Settings", UnityEngine::Color(0.941f, 0.188f, 0.188f, 1.0f));
        AddHoverHint(CreateToggle(scoreDetailsContainer->get_transform(), "Toggle Advanced Score Details", GET_VALUE(enablePopup), 
        [](bool value) {    
            getScorePercentageConfig().enablePopup.SetValue(value);
            toggleToggles(value);
        } )->get_gameObject(), "replaces the score bar in the menu with a useless popup UI");

        ppToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display PP", GET_VALUE(uiPP), 
            [](bool value) {
                getScorePercentageConfig().uiPP.SetValue(value);
                if (!scoreDetailsUI) return;
                scoreDetailsUI->modalSettingsChanged = true;
            });
        
        AddHoverHint(ppToggle->get_gameObject(), "Displays amount of pp given by the score if the map is ranked (playing with positive modifiers will make this value innacurate)");

        playCountToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Play Count", GET_VALUE(uiPlayCount), 
            [](bool value) {
                getScorePercentageConfig().uiPlayCount.SetValue(value);
                if (!scoreDetailsUI) return;
                scoreDetailsUI->modalSettingsChanged = true;
            });

        AddHoverHint(playCountToggle->get_gameObject(), "Displays number of times the map has been played");    

        missCountToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Miss Count", GET_VALUE(uiMissCount), 
            [](bool value) {
                getScorePercentageConfig().uiMissCount.SetValue(value);
                if (!scoreDetailsUI) return;
                scoreDetailsUI->modalSettingsChanged = true;
            });

        AddHoverHint(missCountToggle->get_gameObject(), "Displays number of misses");

        badCutCountToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Bad Cut Count", GET_VALUE(uiBadCutCount), 
            [](bool value) {
                getScorePercentageConfig().uiBadCutCount.SetValue(value);
                if (!scoreDetailsUI) return;
                scoreDetailsUI->modalSettingsChanged = true;
            });

        AddHoverHint(badCutCountToggle->get_gameObject(), "Displays number of bad cuts");

        pauseCountToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Pause Count", GET_VALUE(uiPauseCount), 
            [](bool value) {
                getScorePercentageConfig().uiPauseCount.SetValue(value);
                if (!scoreDetailsUI) return;
                scoreDetailsUI->modalSettingsChanged = true;
            });

        AddHoverHint(pauseCountToggle->get_gameObject(), "Displays number of times the game was paused during the level");

        datePlayedToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Date Played", GET_VALUE(uiDatePlayed), 
            [](bool value) {
                getScorePercentageConfig().uiDatePlayed.SetValue(value);
                if (!scoreDetailsUI) return;
                scoreDetailsUI->modalSettingsChanged = true;
            });

        AddHoverHint(datePlayedToggle->get_gameObject(), "Displays the date on which the most recent score was set");

        alwaysOnToggle = CreateToggle(scoreDetailsContainer->get_transform(), "UI always open", GET_VALUE(alwaysOpen), 
            [](bool value) {
                getScorePercentageConfig().alwaysOpen.SetValue(value);
                if (!scoreDetailsUI) return;
                scoreDetailsUI->modalSettingsChanged = true;
            });

        AddHoverHint(alwaysOnToggle->get_gameObject(), "UI Popup is always popped up");

        // if (FUCKINGBEATSAVIORSUCKMYCOCK) alwaysOnToggle->get_transform()->get_parent()->get_gameObject()->SetActive(false);
        if (!GET_VALUE(enablePopup)) toggleToggles(false);
    }
}