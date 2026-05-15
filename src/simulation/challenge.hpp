#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace memory_playground
{
enum class ChallengeState
{
    Active,
    Completed,
    Failed
};

struct Challenge
{
    std::string title;
    std::string target;
    std::string progress;
    float progressAmount = 0.0f;
    ChallengeState state = ChallengeState::Active;
};

class ChallengeSystem
{
public:
    void reset()
    {
        challenges.clear();
    }

    void update(int totalAccesses,
                int cacheMisses,
                float hitRate,
                float averageCycles,
                int consecutiveRegisterLoads)
    {
        challenges.clear();
        challenges.push_back(makeHitRateChallenge(hitRate, totalAccesses));
        challenges.push_back(makeAverageCostChallenge(averageCycles, totalAccesses));
        challenges.push_back(makeRegisterLoadChallenge(consecutiveRegisterLoads));
        challenges.push_back(makeMissBudgetChallenge(totalAccesses, cacheMisses));
    }

    const std::vector<Challenge>& getChallenges() const
    {
        return challenges;
    }

private:
    static Challenge makeHitRateChallenge(float hitRate, int totalAccesses)
    {
        Challenge challenge{"Spatial locality", "Reach >95% hit rate", "", 0.0f, ChallengeState::Active};
        challenge.progress = std::to_string(static_cast<int>(hitRate)) + "%";
        challenge.progressAmount = hitRate / 95.0f;
        if (totalAccesses >= 32 && hitRate > 95.0f)
        {
            challenge.state = ChallengeState::Completed;
        }
        return challenge;
    }

    static Challenge makeAverageCostChallenge(float averageCycles, int totalAccesses)
    {
        Challenge challenge{"Low average cost", "Stay under 10 cycles/access", "", 0.0f, ChallengeState::Active};
        challenge.progress = std::to_string(static_cast<int>(averageCycles)) + " cycles";
        challenge.progressAmount = totalAccesses == 0 ? 0.0f : std::clamp(10.0f / std::max(10.0f, averageCycles), 0.0f, 1.0f);
        if (totalAccesses >= 32)
        {
            challenge.state = averageCycles < 10.0f ? ChallengeState::Completed : ChallengeState::Failed;
        }
        return challenge;
    }

    static Challenge makeRegisterLoadChallenge(int consecutiveRegisterLoads)
    {
        Challenge challenge{"Register streak", "Load 10 values in a row", "", 0.0f, ChallengeState::Active};
        challenge.progress = std::to_string(consecutiveRegisterLoads) + "/10";
        challenge.progressAmount = static_cast<float>(consecutiveRegisterLoads) / 10.0f;
        if (consecutiveRegisterLoads >= 10)
        {
            challenge.state = ChallengeState::Completed;
        }
        return challenge;
    }

    static Challenge makeMissBudgetChallenge(int totalAccesses, int cacheMisses)
    {
        constexpr int missBudget = 12;
        Challenge challenge{"64-access budget", "64 accesses with <12 misses", "", 0.0f, ChallengeState::Active};
        challenge.progress = std::to_string(totalAccesses) + "/64, misses " + std::to_string(cacheMisses);
        challenge.progressAmount = static_cast<float>(totalAccesses) / 64.0f;
        if (totalAccesses >= 64)
        {
            challenge.state = cacheMisses < missBudget ? ChallengeState::Completed : ChallengeState::Failed;
        }
        return challenge;
    }

    std::vector<Challenge> challenges;
};
}
