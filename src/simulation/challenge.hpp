#pragma once

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
        Challenge challenge{"Spatial locality", "Reach >95% hit rate", "", ChallengeState::Active};
        challenge.progress = std::to_string(static_cast<int>(hitRate)) + "%";
        if (totalAccesses >= 32 && hitRate > 95.0f)
        {
            challenge.state = ChallengeState::Completed;
        }
        return challenge;
    }

    static Challenge makeAverageCostChallenge(float averageCycles, int totalAccesses)
    {
        Challenge challenge{"Low average cost", "Stay under 10 cycles/access", "", ChallengeState::Active};
        challenge.progress = std::to_string(static_cast<int>(averageCycles)) + " cycles";
        if (totalAccesses >= 32)
        {
            challenge.state = averageCycles < 10.0f ? ChallengeState::Completed : ChallengeState::Failed;
        }
        return challenge;
    }

    static Challenge makeRegisterLoadChallenge(int consecutiveRegisterLoads)
    {
        Challenge challenge{"Register streak", "Load 10 values in a row", "", ChallengeState::Active};
        challenge.progress = std::to_string(consecutiveRegisterLoads) + "/10";
        if (consecutiveRegisterLoads >= 10)
        {
            challenge.state = ChallengeState::Completed;
        }
        return challenge;
    }

    static Challenge makeMissBudgetChallenge(int totalAccesses, int cacheMisses)
    {
        constexpr int missBudget = 12;
        Challenge challenge{"64-access budget", "64 accesses with <12 misses", "", ChallengeState::Active};
        challenge.progress = std::to_string(totalAccesses) + "/64, misses " + std::to_string(cacheMisses);
        if (totalAccesses >= 64)
        {
            challenge.state = cacheMisses < missBudget ? ChallengeState::Completed : ChallengeState::Failed;
        }
        return challenge;
    }

    std::vector<Challenge> challenges;
};
}
