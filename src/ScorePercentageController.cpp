#include "ScorePercentageController.hpp"
#include "Utils/DelegateUtils.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/PartyFreePlayFlowCoordinator.hpp"
#include "bsml/shared/Helpers/creation.hpp"
#include "GlobalNamespace/ScoreModel.hpp"
#include "Utils/MapUtils.hpp"
#include "Utils/ScoreUtils.hpp"
#include "ScorePercentageConfig.hpp"
#include "GlobalNamespace/ScoreFormatter.hpp"
#include "Utils/TaskCoroutine.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "GlobalNamespace/LevelCompletionResults.hpp"
#include "GlobalNamespace/PlayerLevelStatsData.hpp"
#include "BGLib/Polyglot/LocalizedTextMeshProUGUI.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/Polyglot/LocalizationModel.hpp"
#include "Localisation.hpp"
#include "main.hpp"

DEFINE_TYPE(ScorePercentage, ScorePercentageController);

using namespace GlobalNamespace;
using namespace DelegateUtils;
using namespace UnityEngine;
using namespace ScorePercentage::Utils;
using namespace BGLib::Polyglot;

namespace ScorePercentage {

    ScorePercentageController* ScorePercentageController::_instance;

    void ScorePercentageController::ctor(ResultsViewController* view, PlayerDataModel* model, PartyFreePlayFlowCoordinator* flow) {
        INVOKE_CTOR();
        resultsView = view;
        playerDataModel = model;
        partyFlow = flow;
    }

    void ScorePercentageController::Initialise() {
        _instance = this;
        resultsView->___didActivateEvent += DelegateW{&ScorePercentageController::ResultsViewDidActivate, this};
        Localisation::AddScorePercentageLocalisation();
    }

    StringW ScorePercentageController::GetLocalisationKey(TMPro::TextMeshProUGUI* rankTitle) {
        auto localisedText = rankTitle->gameObject->GetComponent<LocalizedTextMeshProUGUI*>();
        if (!localisedText) return "";
        getLogger().info("THINGY KEY: {}", localisedText->Key);
        return localisedText->Key;
    }

    task_coroutine<void> ScorePercentageController::SetPercentageTextAsync(LevelStatsView* levelStatsView, BeatmapKey beatmapKey) {
        int maxScore = co_await MapUtils::getMaxScoreAsync(beatmapKey);
        if (!beatmapKey.Equals(currentLocalSelectedMap)) co_return;
        if (currentLocalScore == 0) co_return;
        else if (maxScore == -1) levelStatsView->_maxRankText->text = "N/A";
        else levelStatsView->_maxRankText->text = fmt::format("{:.2f}%", Utils::CalculatePercentage(maxScore, currentLocalScore));
    }

    void ScorePercentageController::ShowStats(LevelStatsView* levelStatsView, ByRef<BeatmapKey> beatmapKey, PlayerData* playerData) {
        if (!playerData) return;
        currentLocalSelectedMap = *beatmapKey;
        auto rankTitle = levelStatsView->get_transform()->Find("MaxRank/Title")->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (cachedMaxRankKey.empty()) cachedMaxRankKey = static_cast<std::string>(GetLocalisationKey(rankTitle));
        Localisation::SetLocalisationKey(rankTitle, GET_VALUE(showPercentageInMenu) ? Localisation::percentageText.key : cachedMaxRankKey);

        auto* playerLevelStatsData = playerData->GetOrCreatePlayerLevelStatsData(beatmapKey);
        if (!playerLevelStatsData->get_validScore()) currentLocalScore = 0;
        else currentLocalScore = playerLevelStatsData->get_highScore();
        if (GET_VALUE(showPercentageInMenu)) SetPercentageTextAsync(levelStatsView, *beatmapKey);
    }

    void ScorePercentageController::CreateDifferenceTexts() {
        scoreDiffText = UnityEngine::Object::Instantiate(resultsView->_rankText, resultsView->_rankText->get_transform()->get_parent());
        rankDiffText = UnityEngine::Object::Instantiate(resultsView->_rankText, resultsView->_rankText->get_transform()->get_parent());
        auto scoreTextPos = resultsView->_scoreText->get_transform()->get_localPosition();
        auto rankTextPos = resultsView->_rankText->get_transform()->get_localPosition();
        scoreDiffText->get_transform()->set_localPosition(Vector3(scoreTextPos.x - 1.0f, rankTextPos.y - 4.5f, scoreTextPos.z));
        rankDiffText->get_transform()->set_localPosition(Vector3(rankTextPos.x + 1.0f, rankTextPos.y - 4.5f, rankTextPos.z));
        scoreDiffText->set_enableWordWrapping(false);
    }

    void ScorePercentageController::ResultsViewDidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
        int resultScore;
        double resultPercentage;

        // disable score differences in party mode cuz idk how that's supposed to work (still shows rank as percentage tho) 
        bool isParty = partyFlow->isActivated;

        bool isValidScore = currentLocalScore != 0 && !isParty;

    #if !NO_GIMMICK
        // funny thing i wonder if anyone notices
        if (firstActivation) BSML::Helpers::CreateText(resultsView->get_transform(), "<size=150%>KNOBHEAD</size>", Vector2(20, 20));
    #endif

        // Default Info Texts
        std::string rankText = resultsView->_rankText->get_text();
        std::string scoreText = resultsView->_scoreText->get_text();

        // only update stuff if level was cleared
        if (resultsView->_levelCompletionResults->levelEndStateType != LevelCompletionResults::LevelEndStateType::Cleared) return;
        
        if (!scoreDiffText || !rankDiffText) CreateDifferenceTexts();

        resultScore = resultsView->_levelCompletionResults->modifiedScore;
        
        int maxScore = ScoreModel::ComputeMaxMultipliedScoreForBeatmap(resultsView->_transformedBeatmapData);
        resultPercentage = CalculatePercentage(maxScore, resultScore);

        // probably stops stuff from breaking
        resultsView->_rankText->set_autoSizeTextContainer(false);
        resultsView->_rankText->set_enableWordWrapping(false);
        resultsView->_goodCutsPercentageText->set_enableWordWrapping(false);
        
        auto* rankTitleText = resultsView->get_transform()->Find("Container/ClearedInfo/RankTitle")->GetComponentInChildren<TMPro::TextMeshProUGUI*>();
        if (firstActivation) cachedResultsRankKey = static_cast<std::string>(GetLocalisationKey(rankTitleText));
        // rank text
        if (GET_VALUE(showPercentageOnResults)) rankText = "  " + Round(std::abs(resultPercentage), 2) + "<size=45%>%";
        
        Localisation::SetLocalisationKey(rankTitleText, GET_VALUE(showPercentageOnResults) ? Localisation::percentageText.key : cachedResultsRankKey);

        // percentage difference text
        if (GET_VALUE(showPercentageDifference) && isValidScore){
            std::string rankDiff = valueDifferenceString(resultPercentage - Utils::CalculatePercentage(maxScore, currentLocalScore));
            std::string sorry = GET_VALUE(showPercentageOnResults) ? "" : "  ";
            rankText = createScoreText(sorry + rankText);
            rankDiffText->get_gameObject()->SetActive(true);
            rankDiffText->SetText("<size=40%>" + rankDiff + "<size=30%>%", true);
        }
        else rankDiffText->get_gameObject()->SetActive(false);
        resultsView->_rankText->set_text(rankText);

        // score difference text
        if (GET_VALUE(showScoreDifference) && isValidScore)
        {
            resultsView->_newHighScoreText->SetActive(false);
            std::string formatting = "<size=40%>" + ((resultScore - currentLocalScore >= 0) ? positiveColour + "+" : negativeColour);
            scoreDiffText->get_gameObject()->SetActive(true);
            scoreDiffText->set_text(StringW(formatting) + ScoreFormatter::Format(resultScore - currentLocalScore));
            resultsView->_scoreText->set_text(createScoreText(" " + scoreText));
        }
        else {
            scoreDiffText->get_gameObject()->SetActive(false);
            resultsView->_scoreText->set_text(scoreText);        
        }

    }

    void ScorePercentageController::Dispose() {
        _instance = nullptr;
        resultsView->___didActivateEvent -= DelegateW{&ScorePercentageController::ResultsViewDidActivate, this};
    }

    ScorePercentageController* ScorePercentageController::get_instance() {
        return _instance;
    }
}