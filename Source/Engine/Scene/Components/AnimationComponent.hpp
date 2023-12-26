#pragma once

enum class AnimatedProperty
{
    eTranslation,
	eRotation,
	eScale
};

enum class AnimationInterpolation
{
	eStep,
	eLinear
};

struct AnimationKeyFrame
{
    float timeStamp;
	glm::vec4 value;
};

struct AnimationTrack
{
	entt::entity target;
	AnimatedProperty property;
	AnimationInterpolation interpolation;
	std::vector<AnimationKeyFrame> keyFrames;
};

struct AnimationState
{
	uint32_t active : 1 = false;
	uint32_t looped : 1 = false;
	uint32_t reverse : 1 = false;
	float time = 0.0f;
	float speed = 1.0f;
};

struct Animation2
{
	std::string name;
	AnimationState state;
	std::vector<AnimationTrack> tracks;
	float duration = 0.0f;
};

struct AnimationComponent2
{
    std::vector<Animation2> animations;
};
