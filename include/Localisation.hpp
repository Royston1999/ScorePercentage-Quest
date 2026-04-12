#pragma once

#include "BGLib/Polyglot/LocalizationLanguage.hpp"
#include "BGLib/Polyglot/Localization.hpp"
#include "BGLib/Polyglot/LocalizationModel.hpp"
#include "BGLib/Polyglot/LocalizedTextMeshProUGUI.hpp"
#include "UnityEngine/GameObject.hpp"
#include <unordered_map>
#include <functional>
#include "Utils/EnumUtils.hpp"

template <>
struct std::hash<BGLib::Polyglot::LocalizationLanguage> : public __unary_function<BGLib::Polyglot::LocalizationLanguage, int32_t> {
    int32_t operator()(BGLib::Polyglot::LocalizationLanguage __v) const { return std::hash<int32_t>()(__v.value__); }
};

namespace ScorePercentage::Localisation {

    struct LocalisedString {
        std::string key;
        std::unordered_map<BGLib::Polyglot::LocalizationLanguage, std::u16string> values;
    };

    inline const LocalisedString percentageText {
        "SCOREPERCENTAGE_PERCENTAGE_NAME_TEXT",
        {
            {BGLib::Polyglot::LocalizationLanguage::English, u"Percentage"},
            {BGLib::Polyglot::LocalizationLanguage::Japanese, u"パーセント"},
            {BGLib::Polyglot::LocalizationLanguage::German, u"Prozentsatz"},
            {BGLib::Polyglot::LocalizationLanguage::Korean, u"백분율"},
            {BGLib::Polyglot::LocalizationLanguage::French, u"Pourcentage"},
            {BGLib::Polyglot::LocalizationLanguage::Spanish, u"Porcentaje"}
        }
    };

    inline void AddLocalisedString(const LocalisedString& localisedString) {
        auto model = BGLib::Polyglot::Localization::get_Instance();
        if (model->KeyExist(localisedString.key)) return;
        int langCount = EnumUtils::GetEnumValueNameMap<BGLib::Polyglot::LocalizationLanguage>().size();
        ListW<StringW> localisedStringList = ListW<StringW>::New(langCount);
        localisedStringList->____size = langCount;

        for (auto [lang, text] : localisedString.values) {
            localisedStringList[(int)lang] = text;
        }
        model->AddLocalizedString(localisedString.key, localisedStringList);
    }

    inline void AddScorePercentageLocalisation() {
        AddLocalisedString(percentageText);
    }

    inline void SetLocalisationKey(TMPro::TextMeshProUGUI* text, StringW key) {
        auto localisedText = text->GetComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>();
        if (!localisedText) {
            localisedText = text->gameObject->AddComponent<BGLib::Polyglot::LocalizedTextMeshProUGUI*>();
            localisedText->___localizedComponent = text;
            localisedText->___maintainTextAlignment = true;
        }
        localisedText->Key = key;
    }
}