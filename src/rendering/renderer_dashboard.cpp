#include "renderer.hpp"
#include "rendering/render_style.hpp"
#include "simulation/trace_analysis.hpp"

#include <algorithm>
#include <cstdio>
#include <map>
#include <string>

namespace memory_playground
{
using namespace rendering_detail;

void Renderer::drawMetrics(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Metrics");

    const Metrics& metrics = simulation.getMetrics();

    char buffer[160];
    const float colW = (panel.width - 2.0f * kPanelPad) / 3.0f;
    const float y = panel.y + kHeaderHeight + 4.0f;
    const float labelY = y + 34.0f;

    std::snprintf(buffer, sizeof(buffer), "%d", metrics.totalAccesses);
    DrawText(buffer, static_cast<int>(panel.x + kPanelPad), static_cast<int>(y), 24, kText);
    DrawText("accesses", static_cast<int>(panel.x + kPanelPad), static_cast<int>(labelY), 13, kMutedText);

    std::snprintf(buffer, sizeof(buffer), "%.0f%%", metrics.hitRate());
    DrawText(buffer, static_cast<int>(panel.x + kPanelPad + colW), static_cast<int>(y), 24, kHitColor);
    DrawText("hit rate", static_cast<int>(panel.x + kPanelPad + colW), static_cast<int>(labelY), 13, kMutedText);

    std::snprintf(buffer, sizeof(buffer), "%.1f", metrics.averageCycles());
    DrawText(buffer, static_cast<int>(panel.x + kPanelPad + colW * 2.0f), static_cast<int>(y), 24, kText);
    DrawText("cycles/access", static_cast<int>(panel.x + kPanelPad + colW * 2.0f), static_cast<int>(labelY), 13, kMutedText);
}

void Renderer::drawTimeline(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Advanced Timeline");

    const auto& trace = simulation.getTraceEvents();
    const int bucketCount = std::max(12, static_cast<int>((panel.width - 2.0f * kPanelPad) / 16.0f));
    const TraceAnalysis analysis = analyzeTrace(trace, bucketCount);

    const Rectangle plot{panel.x + kPanelPad, panel.y + 47.0f, panel.width - 2.0f * kPanelPad, 34.0f};
    DrawRectangleRounded(plot, 0.08f, 6, Color{24, 30, 38, 255});

    if (!analysis.buckets.empty())
    {
        const float gap = 2.0f;
        const float itemW = std::max(4.0f, (plot.width - gap * static_cast<float>(analysis.buckets.size() - 1)) / static_cast<float>(analysis.buckets.size()));
        for (int i = 0; i < static_cast<int>(analysis.buckets.size()); ++i)
        {
            const TimelineBucket& bucket = analysis.buckets[static_cast<std::size_t>(i)];
            const float pressure = bucket.cachePressure();
            const float height = 8.0f + pressure * 22.0f;
            const float x = plot.x + static_cast<float>(i) * (itemW + gap);
            const Rectangle marker{x, plot.y + plot.height - height - 2.0f, itemW, height};
            const Color base = bucket.misses > bucket.hits ? kMissColor : kHitColor;
            DrawRectangleRounded(marker, 0.3f, 5, fadeTo(base, kEvictColor, bucket.evictions > 0 ? 0.35f : 0.0f));
            if (bucket.evictions > 0)
            {
                DrawCircle(static_cast<int>(x + itemW * 0.5f), static_cast<int>(marker.y - 3.0f), 2.0f, kEvictColor);
            }
        }
    }

    char label[160];
    if (analysis.latest.valid)
    {
        std::snprintf(label, sizeof(label), "tick %d | addr %02d | line %02d-%02d | L1 slot %d | %s | %d cycles%s",
                      analysis.latest.tick,
                      analysis.latest.address,
                      analysis.latest.cacheLineStart,
                      analysis.latest.cacheLineEnd,
                      analysis.latest.cacheSlot,
                      analysis.latest.hit ? "hit" : "miss",
                      analysis.latest.cycles,
                      analysis.latest.eviction ? " | eviction" : "");
    }
    else
    {
        std::snprintf(label, sizeof(label), "No trace events yet. Step or unpause the simulation.");
    }
    DrawText(label, static_cast<int>(panel.x + kPanelPad), static_cast<int>(panel.y + panel.height - 25.0f), 13, kMutedText);
    DrawText("pressure", static_cast<int>(panel.x + panel.width - 178.0f), static_cast<int>(panel.y + 17.0f), 13, kMutedText);
    DrawText("hit", static_cast<int>(panel.x + panel.width - 103.0f), static_cast<int>(panel.y + 17.0f), 13, kHitColor);
    DrawText("miss", static_cast<int>(panel.x + panel.width - 64.0f), static_cast<int>(panel.y + 17.0f), 13, kMissColor);
}

void Renderer::drawPerformanceGraph(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Recent Hit Rate");

    const auto& samples = simulation.getPerformanceHistory().getSamples();
    const Rectangle plot{panel.x + kPanelPad, panel.y + 50.0f, panel.width - 2.0f * kPanelPad, panel.height - 68.0f};
    DrawRectangleRounded(plot, 0.04f, 6, Color{24, 30, 38, 255});
    DrawText("100%", static_cast<int>(plot.x), static_cast<int>(plot.y - 18.0f), 12, kMutedText);
    DrawText("0%", static_cast<int>(plot.x), static_cast<int>(plot.y + plot.height + 4.0f), 12, kMutedText);
    DrawLine(static_cast<int>(plot.x), static_cast<int>(plot.y + plot.height * 0.5f), static_cast<int>(plot.x + plot.width), static_cast<int>(plot.y + plot.height * 0.5f), Fade(kLineColor, 0.45f));

    if (samples.size() >= 2)
    {
        for (int i = 1; i < static_cast<int>(samples.size()); ++i)
        {
            const float prevX = plot.x + plot.width * static_cast<float>(i - 1) / static_cast<float>(kPerformanceHistorySize - 1);
            const float nextX = plot.x + plot.width * static_cast<float>(i) / static_cast<float>(kPerformanceHistorySize - 1);
            const float prevY = plot.y + plot.height * (1.0f - samples[static_cast<std::size_t>(i - 1)].hitRate / 100.0f);
            const float nextY = plot.y + plot.height * (1.0f - samples[static_cast<std::size_t>(i)].hitRate / 100.0f);
            DrawLineEx(Vector2{prevX, prevY}, Vector2{nextX, nextY}, 2.0f, kHitColor);
        }
    }

    char label[96];
    std::snprintf(label, sizeof(label), "%.1f cycles/access", simulation.getMetrics().averageCycles());
    DrawText(label, static_cast<int>(panel.x + panel.width - 146.0f), static_cast<int>(panel.y + 18.0f), 13, kMutedText);
}

void Renderer::drawSettings(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Experiment");

    const auto& settings = simulation.getSettings();
    char line[128];
    const float labelX = panel.x + kPanelPad;
    const float trackX = panel.x + 146.0f;
    const float valueX = panel.x + panel.width - 54.0f;
    const float trackW = panel.width - 176.0f;
    const char* labels[] = {"Speed", "Line size", "L1 lines", "Hit", "Miss"};
    const char* hints[] = {"Up/Down", "[ / ]", "- / =", "H / J", "N / M"};
    const float amounts[] = {
        normalized(settings.accessesPerSecond, 0.5f, 12.0f),
        normalized(static_cast<float>(settings.cacheLineSize), static_cast<float>(kMinCacheLineSize), static_cast<float>(kMaxCacheLineSize)),
        normalized(static_cast<float>(settings.cacheLineCount), static_cast<float>(kMinCacheLineCount), static_cast<float>(kMaxCacheLineCount)),
        normalized(static_cast<float>(settings.hitCycles), static_cast<float>(kMinHitCycles), static_cast<float>(kMaxHitCycles)),
        normalized(static_cast<float>(settings.missCycles), static_cast<float>(kMinMissCycles), static_cast<float>(kMaxMissCycles))
    };
    const Color colors[] = {kText, kCacheColor, kCacheColor, kHitColor, kMissColor};

    BeginScissorMode(
        static_cast<int>(panel.x),
        static_cast<int>(panel.y),
        static_cast<int>(panel.width),
        static_cast<int>(panel.height)
    );

    for (int i = 0; i < kSettingsRowCount; ++i)
    {
        const float y = panel.y + kSettingsTopOffset + static_cast<float>(i) * kSettingsRowHeight;
        DrawText(labels[i], static_cast<int>(labelX), static_cast<int>(y - 6.0f), 14, kText);
        DrawText(hints[i], static_cast<int>(labelX), static_cast<int>(y + 9.0f), 11, kMutedText);
        drawSlider(Rectangle{trackX, y, trackW, 8.0f}, amounts[i], colors[i]);

        if (i == 0)
        {
            std::snprintf(line, sizeof(line), "%.1f", settings.accessesPerSecond);
        }
        else if (i == 1)
        {
            std::snprintf(line, sizeof(line), "%d", settings.cacheLineSize);
        }
        else if (i == 2)
        {
            std::snprintf(line, sizeof(line), "%d", settings.cacheLineCount);
        }
        else if (i == 3)
        {
            std::snprintf(line, sizeof(line), "%d", settings.hitCycles);
        }
        else
        {
            std::snprintf(line, sizeof(line), "%d", settings.missCycles);
        }
        DrawText(line, static_cast<int>(valueX), static_cast<int>(y - 6.0f), 14, colors[i]);
    }

    EndScissorMode();
}

void Renderer::drawChallenges(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Challenges");

    const auto& challenges = simulation.getChallenges().getChallenges();
    if (challenges.empty())
    {
        return;
    }

    const float contentX = panel.x + kPanelPad;
    const float contentY = panel.y + kHeaderHeight;
    const float contentW = panel.width - 2.0f * kPanelPad;

    BeginScissorMode(
        static_cast<int>(panel.x),
        static_cast<int>(panel.y),
        static_cast<int>(panel.width),
        static_cast<int>(panel.height)
    );

    for (int i = 0; i < static_cast<int>(challenges.size()); ++i)
    {
        const Challenge& challenge = challenges[static_cast<std::size_t>(i)];

        Color stateColor = kMutedText;
        Color cardFill{24, 30, 38, 255};
        const char* stateLabel = "Active";

        if (challenge.state == ChallengeState::Completed)
        {
            stateColor = kHitColor;
            stateLabel = "Done";
        }
        else if (challenge.state == ChallengeState::Failed)
        {
            stateColor = Color{170, 118, 118, 255};
            cardFill = Color{25, 27, 32, 255};
            stateLabel = "Fail";
        }

        const Rectangle card{
            contentX,
            contentY + static_cast<float>(i) * (kChallengeRowHeight + kChallengeRowGap),
            contentW,
            kChallengeRowHeight
        };

        DrawRectangleRounded(card, 0.08f, 8, cardFill);
        DrawRectangleRoundedLines(
            card,
            0.08f,
            8,
            Fade(stateColor, challenge.state == ChallengeState::Failed ? 0.15f : 0.32f)
        );

        const float innerX = card.x + 10.0f;
        const float innerW = card.width - 20.0f;
        const float statusW = 54.0f;
        const float titleW = innerW - statusW - 10.0f;

        const Color quietText = challenge.state == ChallengeState::Failed ? Fade(kMutedText, 0.65f) : kMutedText;
        const Color titleColor = challenge.state == ChallengeState::Failed ? Fade(kMutedText, 0.76f) : kText;

        std::string progressText = challenge.progress;
        int current = 0;
        int target = 0;
        if (std::sscanf(challenge.progress.c_str(), "%d/%d", &current, &target) == 2 && target > 0)
        {
            current = std::clamp(current, 0, target);
            char clamped[32];
            std::snprintf(clamped, sizeof(clamped), "%d/%d", current, target);
            progressText = clamped;
        }

        float progressAmount = std::clamp(challenge.progressAmount, 0.0f, 1.0f);
        if (challenge.state == ChallengeState::Completed)
        {
            progressAmount = 1.0f;
        }

        const float titleY = card.y + 8.0f;
        const float progressY = card.y + 27.0f;
        const float barY = card.y + 45.0f;

        BeginScissorMode(
            static_cast<int>(innerX),
            static_cast<int>(titleY - 1.0f),
            static_cast<int>(titleW),
            16
        );
        DrawText(challenge.title.c_str(), static_cast<int>(innerX), static_cast<int>(titleY), 12, titleColor);
        EndScissorMode();

        DrawText(
            stateLabel,
            static_cast<int>(card.x + card.width - statusW - 4.0f),
            static_cast<int>(titleY),
            11,
            stateColor
        );

        DrawText(
            progressText.c_str(),
            static_cast<int>(innerX),
            static_cast<int>(progressY),
            10,
            quietText
        );

        const Rectangle bar{innerX, barY, innerW, 4.0f};
        DrawRectangleRounded(bar, 0.6f, 6, Color{45, 53, 64, 255});
        DrawRectangleRounded(
            Rectangle{bar.x, bar.y, bar.width * progressAmount, bar.height},
            0.6f,
            6,
            challenge.state == ChallengeState::Failed ? Fade(stateColor, 0.45f) : stateColor
        );
    }

    EndScissorMode();
}

void Renderer::drawLearningFeedback(const SimulationState& simulation, Rectangle panel) const
{
    drawPanel(panel, "Learn");

    const char* conceptTitle = "Spatial locality";
    const char* modeText = "Nearby array elements reuse cache lines.";
    const char* detailText = "One miss can make the next cells cheap.";
    const std::string modeName = simulation.getPattern().name();
    if (modeName.find("Random") != std::string::npos)
    {
        conceptTitle = "Poor locality";
        modeText = "Random jumps rarely reuse loaded lines.";
        detailText = "The cache cannot build a rhythm.";
    }
    else if (modeName.find("Linked") != std::string::npos)
    {
        conceptTitle = "Pointer chasing";
        modeText = "Each node reveals the next address late.";
        detailText = "The CPU cannot easily look ahead.";
    }

    const Rectangle conceptCard{panel.x + kPanelPad, panel.y + kHeaderHeight, panel.width - 2.0f * kPanelPad, 58.0f};
    DrawRectangleRounded(conceptCard, 0.08f, 8, Color{24, 30, 38, 255});
    DrawText(conceptTitle, static_cast<int>(conceptCard.x + 12.0f), static_cast<int>(conceptCard.y + 8.0f), 15, kText);
    DrawText(modeText, static_cast<int>(conceptCard.x + 12.0f), static_cast<int>(conceptCard.y + 30.0f), 13, kMutedText);

    const auto& event = simulation.getLastEvent();
    const Rectangle eventCard{panel.x + kPanelPad, conceptCard.y + conceptCard.height + 10.0f, panel.width - 2.0f * kPanelPad, 58.0f};
    DrawRectangleRounded(eventCard, 0.08f, 8, Color{24, 30, 38, 255});
    if (!event)
    {
        DrawText("Last access", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 8.0f), 15, kText);
        DrawText("Waiting for data movement.", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 30.0f), 13, kMutedText);
        return;
    }

    if (event->hit)
    {
        DrawText("Hit", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 8.0f), 15, kHitColor);
        DrawText("Reused a line already in L1.", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 30.0f), 13, kMutedText);
    }
    else
    {
        DrawText("Miss", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 8.0f), 15, kMissColor);
        DrawText("Loaded a full line from RAM.", static_cast<int>(eventCard.x + 12.0f), static_cast<int>(eventCard.y + 30.0f), 13, kMutedText);
    }

    if (event->evictedLineStart)
    {
        DrawText("Eviction", static_cast<int>(eventCard.x + eventCard.width - 76.0f), static_cast<int>(eventCard.y + 8.0f), 13, kEvictColor);
    }

    DrawText(detailText, static_cast<int>(panel.x + kPanelPad), static_cast<int>(eventCard.y + eventCard.height + 12.0f), 13, kMutedText);
}

void Renderer::drawFooter(const SimulationState& simulation, Rectangle panel) const
{
    DrawRectangleRounded(panel, 0.14f, 10, Color{24, 30, 38, 240});
    DrawRectangleRoundedLines(panel, 0.14f, 10, kPanelBorder);

    const char* runState = simulation.paused ? "Paused" : "Running";
    DrawText(runState,
             static_cast<int>(panel.x + 18.0f),
             static_cast<int>(panel.y + 13.0f),
             16,
             simulation.paused ? kEvictColor : kHitColor);

    const char* controls = "Space pause  R reset  1/2/3 modes  Enter step";
    DrawText(controls, static_cast<int>(panel.x + 110.0f), static_cast<int>(panel.y + 14.0f), 14, kMutedText);

    const char* settings = "Drag sliders or use Up/Down  [ ]  -/=  H/J  N/M";
    const int settingsX = static_cast<int>(std::max(panel.x + 520.0f, panel.x + panel.width - static_cast<float>(MeasureText(settings, 14)) - 18.0f));
    if (settingsX + MeasureText(settings, 14) < static_cast<int>(panel.x + panel.width - 8.0f))
    {
        DrawText(settings, settingsX, static_cast<int>(panel.y + 14.0f), 14, kMutedText);
    }
}
}
