#pragma once

#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Color.hpp"
#include <string>
#include "HMUI/TitleViewController.hpp"
#include "HMUI/ViewController.hpp"
#include "Zenject/DiContainer.hpp"

namespace UIUtils
{
    TMPro::TextMeshProUGUI* AddHeader(UnityEngine::Transform* parent, std::string title, UnityEngine::Color color);
    TMPro::TextMeshProUGUI* AddHeader(UnityEngine::Transform* parent, std::string title, UnityEngine::Color leftColor, UnityEngine::Color rightColor);
}