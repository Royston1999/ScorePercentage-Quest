#include "main.hpp"

#include "MultiplayerHooks.hpp"

#include "GlobalNamespace/ScoreFormatter.hpp"
#include "GlobalNamespace/ResultsViewController.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "GlobalNamespace/MenuTransitionsHelper.hpp"
#include "GlobalNamespace/GamePause.hpp"
#include "GlobalNamespace/MainMenuViewController.hpp"
#include "GlobalNamespace/GameplayCoreSceneSetupData.hpp"
#include "GlobalNamespace/LevelStatsView.hpp"
#include "GlobalNamespace/PlayerDataModel.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "GlobalNamespace/PlatformLeaderboardViewController.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "GlobalNamespace/GameplayModifiersModelSO.hpp"
#include "GlobalNamespace/LevelCompletionResultsHelper.hpp"
#include "GlobalNamespace/PauseMenuManager.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"

#include "bsml/shared/BSML.hpp"
#include "bsml/shared/Helpers/creation.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"

#include "bsml/shared/Helpers/getters.hpp"
#include "ScorePercentageController.hpp"
#include "lapiz/shared/zenject/Zenjector.hpp"
#include "Zenject/ScopeConcreteIdArgConditionCopyNonLazyBinder.hpp"
#include "ScorePercentageConfig.hpp"

#include "BGLib/Polyglot/LocalizationModel.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "UnityEngine/TextAsset.hpp"
#include "BGLib/Polyglot/LocalizationAsset.hpp"
#include "BGLib/Polyglot/LocalizationImporter.hpp"
#include "System/Globalization/CultureInfo.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"


using namespace UnityEngine::UI;
using namespace UnityEngine;
using namespace GlobalNamespace;

modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

// Called at the early stages of game loading
SCORE_PERCENTAGE_EXPORT_FUNC void setup(CModInfo& info) {
    info = modInfo.to_c();
    getLogger().info("Completed setup!");
}

MAKE_HOOK_MATCH(Menu, static_cast<void(LevelStatsView::*)(ByRef<BeatmapKey>, PlayerData*)>(&LevelStatsView::ShowStats), void, LevelStatsView* self, ByRef<BeatmapKey> beatmapKey, PlayerData* playerData) {
    Menu(self, beatmapKey, playerData);
    ScorePercentage::ScorePercentageController::get_instance()->ShowStats(self, beatmapKey, playerData);
}


void DidActivate(HMUI::ViewController* self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    using namespace BSML::Lite;
    if (!firstActivation) return;
    auto container = CreateVerticalLayoutGroup(self->transform);
    auto showPerc = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showPercentageOnResults);
    AddHoverHint(showPerc->get_gameObject(), "displays your rank as a percentage on the results screen");
    auto percDiff = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showPercentageDifference);
    AddHoverHint(percDiff->get_gameObject(), "displays the score difference as a percentage compared to your previous high score");
    auto scoreDiff = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showScoreDifference);
    AddHoverHint(scoreDiff->get_gameObject(), "displays the score difference compared to your previous high score");
    auto missDiff = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showMissDifference);
    AddHoverHint(missDiff->get_gameObject(), "displays the miss difference compared to your previous high score");
    auto percMenu = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().showPercentageInMenu);
    AddHoverHint(percMenu->get_gameObject(), "displays your rank as a percentage in the stats view below the leaderboard");
    auto multiPerc = AddConfigValueToggle(container->get_transform(), getScorePercentageConfig().multiShowPercentageOnResults);
    AddHoverHint(multiPerc->get_gameObject(), "displays player ranks as a percentage on the multiplayer results screen");
}

// Called later on in the game loading - a good time to install function hooks
SCORE_PERCENTAGE_EXPORT_FUNC void late_load() {
    il2cpp_functions::Init();
    BSML::Init();
    custom_types::Register::AutoRegister();
    getScorePercentageConfig().Init(modInfo);
    BSML::Register::RegisterMainMenu("Score Percentage", "Score Percentage", "percent your scores!", &DidActivate);
    getLogger().info("Installing hooks...");
    INSTALL_HOOK(getLogger(), Menu);
    
    ScorePercentage::MultiplayerHooks::InstallHooks();
    getLogger().info("Installed all hooks!");

    auto zenjector = Lapiz::Zenject::Zenjector::Get();
    zenjector->Install(Lapiz::Zenject::Location::Menu, [](Zenject::DiContainer* container) {
        container->BindInterfacesAndSelfTo<ScorePercentage::ScorePercentageController*>()->AsSingle();
    });
}