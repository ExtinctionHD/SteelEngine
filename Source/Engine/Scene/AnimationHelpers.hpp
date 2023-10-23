#pragma once

#include "Engine/Scene/Components/AnimationComponent.hpp"

#include <glm/vec4.hpp>

namespace AnimationHelpers
{
	TrackUid GenerateTrackUid();
	AnimationUid GenerateAnimationUid();

	eKeyFrameInterpolation ParseInterpolationType(const std::string& str);

	glm::vec4 FindValueAt(const KeyFrameAnimationTrack& track, float time, bool isLooped);
	glm::quat FindQuatValueAt(const KeyFrameAnimationTrack& track, float time, bool isLooped);
    bool IsTrackFinished(const KeyFrameAnimationTrack& track, float time);
}
