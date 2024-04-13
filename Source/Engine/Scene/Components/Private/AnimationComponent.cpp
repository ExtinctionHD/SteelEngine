#include "Engine/Scene/Components/AnimationComponent.hpp"

void Animation::Play()
{
    if (time == duration)
    {
        Reset();
    }

    active = true;
}

void Animation::Pause()
{
    active = false;
}

void Animation::Reset()
{
    time = 0.0f;
}
