#include "ScoreDetailsUIViewController.hpp"
#include "SettingsFlowCoordinator.hpp"
#include "ScoreDetailsConfig.hpp"
#include "UnityEngine/RectOffset.hpp"

DEFINE_TYPE(ScoreDetailsUI::Views, ScoreDetailsUIViewController);
UnityEngine::UI::Toggle* ppToggle = nullptr;
UnityEngine::UI::Toggle* playCountToggle = nullptr;
UnityEngine::UI::Toggle* missCountToggle = nullptr;
UnityEngine::UI::Toggle* badCutCountToggle = nullptr;
UnityEngine::UI::Toggle* pauseCountToggle = nullptr;
UnityEngine::UI::Toggle* datePlayedToggle = nullptr;
UnityEngine::UI::VerticalLayoutGroup* scoreDetailsContainer;
Array<TMPro::TextMeshProUGUI*>* toggleText;

void toggleToggles(bool value){
    toggleText = scoreDetailsContainer->get_transform()->GetComponentsInChildren<TMPro::TextMeshProUGUI*>();
    ppToggle->set_interactable(value);
    playCountToggle->set_interactable(value);
    missCountToggle->set_interactable(value);
    badCutCountToggle->set_interactable(value);
    pauseCountToggle->set_interactable(value);
    datePlayedToggle->set_interactable(value);
    
    if (!value) {
        for(int i=0; i<toggleText->get_Length(); i++) 
            if ((i!=0 && i%3==0) || (i!=1 && i%3==2 && (*toggleText)[i]->get_alpha()>0))
                (*toggleText)[i]->set_color(UnityEngine::Color::get_gray());
    }
    else { 
        for(int i=0; i<toggleText->get_Length(); i++) 
            if ((i!=0 && i%3==0) || (i!=1 && i%3==2 && (*toggleText)[i]->get_alpha()>0))
                (*toggleText)[i]->set_color(UnityEngine::Color::get_white());
    }
}

void ScoreDetailsUI::Views::ScoreDetailsUIViewController::DidActivate(
    bool firstActivation,
    bool addedToHierarchy,
    bool screenSystemEnabling
) {
    using namespace GlobalNamespace;
    using namespace UnityEngine;
    using namespace QuestUI::BeatSaberUI;
    using namespace UnityEngine::UI;

    if (firstActivation) {
        scoreDetailsContainer = QuestUI::BeatSaberUI::CreateVerticalLayoutGroup(get_transform());
        AddHoverHint(CreateToggle(scoreDetailsContainer->get_transform(), "Toggle Advanced Score Details", ScoreDetails::config.MenuHighScore, 
        [](bool value) {    
            setBool(getConfig().config, "Menu Highscore Percentage", value, false); getConfig().Write();
            ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
            toggleToggles(value);
        } )->get_gameObject(), "replaces the score bar in the menu with a useless popup UI");

        ppToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display PP", ScoreDetails::config.uiPP, 
            [](bool value) {
                setBool(getConfig().config, "uiPP", value, false); getConfig().Write();
                ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
                ScoreDetails::modalSettingsChanged = true;
            });
        
        AddHoverHint(ppToggle->get_gameObject(), "Displays amount of pp given by the score if the map is ranked (playing with positive modifiers will make this value innacurate)");

        playCountToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Play Count", ScoreDetails::config.uiPlayCount, 
            [](bool value) {
                setBool(getConfig().config, "uiPlayCount", value, false); getConfig().Write();
                ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
                ScoreDetails::modalSettingsChanged = true;
            });

        AddHoverHint(playCountToggle->get_gameObject(), "Displays number of times the map has been played");    

        missCountToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Miss Count", ScoreDetails::config.uiMissCount, 
            [](bool value) {
                setBool(getConfig().config, "uiMissCount", value, false); getConfig().Write();
                ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
                ScoreDetails::modalSettingsChanged = true;
            });

        AddHoverHint(missCountToggle->get_gameObject(), "Displays number of misses");

        badCutCountToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Bad Cut Count", ScoreDetails::config.uiBadCutCount, 
            [](bool value) {
                setBool(getConfig().config, "uiBadCutCount", value, false); getConfig().Write();
                ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
                ScoreDetails::modalSettingsChanged = true;
            });

        AddHoverHint(badCutCountToggle->get_gameObject(), "Displays number of bad cuts");

        pauseCountToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Pause Count", ScoreDetails::config.uiPauseCount, 
            [](bool value) {
                setBool(getConfig().config, "uiPauseCount", value, false); getConfig().Write();
                ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
                ScoreDetails::modalSettingsChanged = true;
            });

        AddHoverHint(pauseCountToggle->get_gameObject(), "Displays number of times the game was paused during the level");

        datePlayedToggle = CreateToggle(scoreDetailsContainer->get_transform(), "Display Date Played", ScoreDetails::config.uiDatePlayed, 
            [](bool value) {
                setBool(getConfig().config, "uiDatePlayed", value, false); getConfig().Write();
                ConfigHelper::LoadConfig(ScoreDetails::config, getConfig().config);
                ScoreDetails::modalSettingsChanged = true;
            });

        AddHoverHint(datePlayedToggle->get_gameObject(), "Displays the date on which the most recent score was set");

        if (!ScoreDetails::config.MenuHighScore) toggleToggles(false);
    }
}