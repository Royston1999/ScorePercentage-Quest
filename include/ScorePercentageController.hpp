#pragma once

#include "Utils/TaskCoroutine.hpp"
#include "custom-types/shared/macros.hpp"
#include "GlobalNamespace/ResultsViewController.hpp"
#include "Zenject/IInitializable.hpp"
#include "System/IDisposable.hpp"
#include "lapiz/shared/macros.hpp"
#include "GlobalNamespace/LevelStatsView.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"

DECLARE_CLASS_CODEGEN_INTERFACES(ScorePercentage, ScorePercentageController, System::Object, Zenject::IInitializable*, System::IDisposable*) {
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::ResultsViewController*, resultsView);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::PlayerDataModel*, playerDataModel);
    DECLARE_INSTANCE_FIELD_PRIVATE(GlobalNamespace::PartyFreePlayFlowCoordinator*, partyFlow);
    DECLARE_CTOR(ctor, GlobalNamespace::ResultsViewController* resultsView, GlobalNamespace::PlayerDataModel* playerDataModel, GlobalNamespace::PartyFreePlayFlowCoordinator* partyFlow);
    DECLARE_SIMPLE_DTOR();

    DECLARE_INSTANCE_METHOD(void, ShowStats, GlobalNamespace::LevelStatsView* levelStatsView, ByRef<GlobalNamespace::BeatmapKey> beatmapKey, GlobalNamespace::PlayerData* playerData);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Initialise, &Zenject::IInitializable::Initialize);
    DECLARE_OVERRIDE_METHOD_MATCH(void, Dispose, &System::IDisposable::Dispose);

    DECLARE_INSTANCE_METHOD(void, ResultsViewDidActivate, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_INSTANCE_METHOD(void, CreateDifferenceTexts);
    DECLARE_INSTANCE_METHOD(StringW, GetLocalisationKey, TMPro::TextMeshProUGUI* rankTitle);

    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, scoreDiffText);
    DECLARE_INSTANCE_FIELD(TMPro::TextMeshProUGUI*, rankDiffText);
    
    public:
    static ScorePercentageController* get_instance();
    private:
    static ScorePercentageController* _instance;
    std::string cachedResultsRankKey;
    std::string cachedMaxRankKey;
    int currentLocalScore;
    GlobalNamespace::BeatmapKey currentLocalSelectedMap;
    task_coroutine<void> SetPercentageTextAsync(GlobalNamespace::LevelStatsView* levelStatsView, GlobalNamespace::BeatmapKey beatmapKey);
};