#include "Engine/Scene/Systems/AnimationSystem.hpp"

#include "Engine/Scene/Scene.hpp"
#include "Engine/Scene/Components/AnimationComponent.hpp"
#include "Engine/Scene/Components/Components.hpp"
#include "Engine/Scene/AnimationHelpers.hpp"
#include "Utils/Assert.hpp"

AnimationSystem::AnimationSystem()
{
}

AnimationSystem::~AnimationSystem()
{
}

void AnimationSystem::Process(Scene& scene, float deltaSeconds)
{
	timeSinceKeyFrameAnimationsUpdate += deltaSeconds;

	if (timeSinceKeyFrameAnimationsUpdate < keyFrameAnimationUpdateStepSeconds)
	{
		return;
	}

	UpdateKeyFrameAnimations(scene, timeSinceKeyFrameAnimationsUpdate);

	timeSinceKeyFrameAnimationsUpdate = 0.0f;
}

void AnimationSystem::UpdateKeyFrameAnimations(Scene& scene, float deltaSeconds)
{
    const AnimationStorageComponent& animationStorage = scene.ctx().get<AnimationStorageComponent>();

    auto view = scene.view<AnimationControlComponent>();

    for (entt::entity entity : view)
    {
        auto& animationControlComponent = view.get<AnimationControlComponent>(entity);
        for (Animation& anim : animationControlComponent.animations)
        {
            if (anim.IsPlaying())
            {
                UpdateAnimation(anim, scene, animationStorage, deltaSeconds);
            }
        }
    }
}

void AnimationSystem::UpdateAnimation(Animation& anim, Scene& scene, const AnimationStorageComponent& animationStorage, float deltaSeconds)
{
    bool isFinished = !anim.isLooped;
    anim.curTime += deltaSeconds * anim.playbackSpeed;

    for (entt::entity animatedEntity : anim.animatedEntities)
    {
        const AnimationComponent* animationData = scene.try_get<const AnimationComponent>(animatedEntity);
        if (!animationData)
        {
            Assert(false);
            continue;
        }
        auto it = animationData->animationTracks.find(anim.uid);
        if (it == animationData->animationTracks.end())
        {
            Assert(false);
            continue;
        }

        const std::vector<TrackUid>& tracks = it->second;
        for (const TrackUid& trackUid : tracks)
        {
            // if gets slow - change animationTracks for map or smth
            auto animIt = std::find_if(animationStorage.animationTracks.begin(), animationStorage.animationTracks.end(),
                [trackUid](const KeyFrameAnimationTrack& track) { return track.uid == trackUid; }
            );
            if (animIt == animationStorage.animationTracks.end())
            {
                Assert(false);
                continue;
            }
            TransformComponent& tc = scene.get<TransformComponent>(animatedEntity);

            UpdateAnimationTrack(*animIt, tc, anim.curTime, anim.isLooped);

            isFinished &= AnimationHelpers::IsTrackFinished(*animIt, anim.curTime);
        }
    }

    if (isFinished)
    {
        anim.Stop();
    }
}

void AnimationSystem::UpdateAnimationTrack(const KeyFrameAnimationTrack& track, TransformComponent& tc, float animCurTime, bool isLooped)
{
    if (track.propName == "translation")
    {
        glm::vec3 translation = AnimationHelpers::FindValueAt(track, animCurTime, isLooped);
        tc.SetLocalTranslation(translation);
    }
    else if (track.propName == "rotation")
    {
        glm::quat rotation = AnimationHelpers::FindQuatValueAt(track, animCurTime, isLooped);
        tc.SetLocalRotation(rotation);
    }
    else if (track.propName == "scale")
    {
        glm::vec3 scale = AnimationHelpers::FindValueAt(track, animCurTime, isLooped); // vec4 to vec3
        tc.SetLocalScale(scale);
    }
}
