#pragma once
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "GlobalNamespace/IReadonlyBeatmapData.hpp"

namespace ScorePercentage::MultiplayerHooks{
    void InstallHooks();
}
static float LerpU(float a, float b, float t){
	return a + (b - a) * t;
}