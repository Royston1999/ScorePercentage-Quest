#pragma once

#include "custom-types/shared/macros.hpp"
#include "SettingsViewController.hpp"
#include "UI/ScoreDetailsUIViewController.hpp"
#include "UI/MultiplayerSettingsViewController.hpp"
#include "HMUI/FlowCoordinator.hpp"
#include "HMUI/ViewController.hpp"
#include "main.hpp"

DECLARE_CLASS_CODEGEN(ScoreDetailsUI, SettingsFlowCoordinator, HMUI::FlowCoordinator,
    DECLARE_INSTANCE_FIELD(ScoreDetailsUI::Views::SettingsViewController*, settingsViewController);
    DECLARE_INSTANCE_FIELD(ScoreDetailsUI::Views::ScoreDetailsUIViewController*, scoreDetailsUIViewController);
    DECLARE_INSTANCE_FIELD(ScoreDetailsUI::Views::MultiplayerSettingsViewController*, multiSettingsViewController);

    DECLARE_OVERRIDE_METHOD(void, DidActivate, il2cpp_utils::FindMethodUnsafe("HMUI", "FlowCoordinator", "DidActivate", 3), bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling);
    DECLARE_OVERRIDE_METHOD(void, BackButtonWasPressed, il2cpp_utils::FindMethodUnsafe("HMUI", "FlowCoordinator", "BackButtonWasPressed", 1), HMUI::ViewController* topViewController);
)