#include "AnimationPlayer.h"
#include <qdebug.h>


using namespace graphics_engine;

AnimationPlayer::AnimationPlayer()
{

}

void graphics_engine::AnimationPlayer::initialize(GraphicsEngineInterface* gei)
{
	mGEI = gei;
}

void AnimationPlayer::update(float dt)
{
	assert(mGEI);
	float modifiedDT = PlaybackSpeed * dt;
	size_t firstTrackIndex = 0;
	std::vector<float> strengthArray(mStack.size());
	for (size_t i = 0; i < mStack.size(); i++)
	{
		auto& track = mStack[i];
		track.elapsedTime += modifiedDT;
		track.loopTime += modifiedDT;

		if (track.loop)
		{
			// Time
			auto animation = track.animationTarget->animation;
			while (track.loop && track.loopTime > animation->inputMax)
			{
				track.loopTime -= animation->inputDuration;
			}

			// Strength
			if (!track.pop || track.elapsedTime < track.blendTime)
			{
				if (track.blendTime > 0.0f)
					strengthArray[i] = std::clamp(track.elapsedTime / track.blendTime, 0.0f, 1.0f);
				else
					strengthArray[i] = 1.0f;
			}
			else
			{
				track.timeLeft -= modifiedDT;
				if (track.blendTime > 0.0f)
					strengthArray[i] = std::clamp(track.timeLeft / track.blendTime, 0.0f, 1.0f);
				else
					strengthArray[i] = 0.0f;
			}
		}

		// Start applying animations from the first track that has strength equals to 1.0
		// The ones before will get overwritten so no point in applying them
		if (strengthArray[i] >= 1.0f) firstTrackIndex = i;
	}

	// Starting from the back, remove the tracks that are done playing
	for (int i = (int)(mStack.size() - 1); i >= 0; i--)
	{
		if (strengthArray[i] == 0.0f)
			mStack.erase(mStack.begin() + i);
	}

	for (size_t i = firstTrackIndex; i < mStack.size(); i++)
	{
		auto& track = mStack[i];
		mGEI->setAnimationTime(*track.animationTarget, track.loopTime, strengthArray[i]);
	}
}

AnimationTarget* AnimationPlayer::get(const std::string& name)
{
	if (mLibrary.find(name) == mLibrary.end())
	{
		mLibrary[name] = AnimationTarget();
	}
	return &mLibrary[name];
}

void AnimationPlayer::push(const std::string& name, float blendTime, bool loop)
{
	auto previousPtr = (mStack.size() >= 1) ? &mStack.back() : nullptr;
	bool inheritLoopTime = (loop && previousPtr);
	float loopTime = inheritLoopTime ? previousPtr->loopTime : 0.0f;
	// This will invalidate 'previousPtr'
	mStack.push_back(AnimationTrack());
	auto& track = mStack.back();
	track.name = name;
	track.animationTarget = get(name);
	track.elapsedTime = 0.0f;
	track.loopTime = loopTime;
	track.blendTime = blendTime;
	track.timeLeft = blendTime;
	track.loop = loop;
	track.pop = false;
}

void AnimationPlayer::pop()
{
	mStack.pop_back();
}

void AnimationPlayer::popSlowly()
{
	if (mStack.empty())
	{
		qWarning() << "Nothing in the animation player to pop";
		return;
	}
	auto& track = mStack.back();
	if (!track.pop)
	{
		track.pop = true;
	}
}

std::string AnimationPlayer::peek()
{
	return mStack.empty() ? "" : mStack.back().name;
}
