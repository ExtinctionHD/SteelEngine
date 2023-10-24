#include "Engine/Scene/Components/AnimationComponent.hpp"
#include "Utils/Assert.hpp"

void Animation::Start()
{
	Assert(!IsPlaying());

	isLooped = false;
	state = eState::Playing;
	curTime = 0.0f;
}

void Animation::StartLooped()
{
	Assert(!IsPlaying());

	isLooped = true;
	state = eState::Playing;
	curTime = 0.0f;
}

void Animation::Stop()
{
	isLooped = false;
	state = eState::Stopped;
}

void Animation::Pause()
{
	state = eState::Paused;
}

void Animation::Unpause()
{
	state = eState::Playing;
}

bool Animation::IsPlaying() const
{
	return state == eState::Playing;
}

bool Animation::IsPaused() const
{
	return state == eState::Paused;
}
