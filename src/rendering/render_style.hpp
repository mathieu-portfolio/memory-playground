#pragma once

#include "app_config.hpp"

#include <algorithm>
#include <cstddef>

#include <raylib.h>

namespace memory_playground::rendering_detail
{
inline constexpr Color kBackground{18, 22, 28, 255};
inline constexpr Color kPanel{29, 35, 44, 255};
inline constexpr Color kPanelBorder{56, 66, 78, 255};
inline constexpr Color kText{229, 236, 244, 255};
inline constexpr Color kMutedText{151, 163, 178, 255};
inline constexpr Color kRamColor{61, 84, 111, 255};
inline constexpr Color kCacheColor{49, 119, 99, 255};
inline constexpr Color kRegisterColor{112, 91, 154, 255};
inline constexpr Color kHitColor{88, 196, 122, 255};
inline constexpr Color kMissColor{222, 101, 101, 255};
inline constexpr Color kEvictColor{226, 177, 76, 255};
inline constexpr Color kLineColor{101, 116, 139, 255};

inline constexpr float kPanelPad = 18.0f;
inline constexpr float kHeaderHeight = kUiHeaderHeight;
inline constexpr float kOuterMargin = kUiOuterMargin;
inline constexpr float kColumnGap = 30.0f;
inline constexpr float kPanelGap = kUiPanelGap;
inline constexpr float kFooterHeight = kUiFooterHeight;
inline constexpr float kRightColumnWidth = 390.0f;
inline constexpr float kMetricsHeight = kUiMetricsHeight;
inline constexpr float kGraphHeight = kUiGraphHeight;
inline constexpr float kSettingsHeight = kUiSettingsHeight;
inline constexpr int kSettingsRowCount = kUiSettingsRowCount;
inline constexpr float kSettingsTopOffset = kUiSettingsTopOffset;
inline constexpr float kSettingsRowHeight = kUiSettingsRowHeight;
inline constexpr float kChallengeRowHeight = kUiChallengeRowHeight;
inline constexpr float kChallengeRowGap = kUiChallengeRowGap;
inline constexpr float kLearnHeight = kUiLearnHeight;
inline constexpr float kLeftTopY = kUiTopY;
inline constexpr float kRegistersHeight = 96.0f;
inline constexpr float kCacheHeight = 160.0f;
inline constexpr float kTimelineHeight = 68.0f;

inline float challengesHeightForCount(std::size_t count)
{
    if (count == 0)
    {
        return kHeaderHeight + 16.0f;
    }

    return kHeaderHeight +
        kChallengeRowHeight * static_cast<float>(count) +
        kChallengeRowGap * static_cast<float>(count - 1) +
        16.0f;
}

inline float normalized(float value, float minValue, float maxValue)
{
    return std::clamp((value - minValue) / (maxValue - minValue), 0.0f, 1.0f);
}

inline void drawSlider(Rectangle track, float amount, Color color)
{
    DrawRectangleRounded(track, 0.5f, 8, Color{45, 53, 64, 255});
    DrawRectangleRounded(Rectangle{track.x, track.y, track.width * amount, track.height}, 0.5f, 8, Fade(color, 0.85f));
    DrawCircle(static_cast<int>(track.x + track.width * amount), static_cast<int>(track.y + track.height * 0.5f), 7.0f, color);
}

inline Vector2 quadraticPoint(Vector2 a, Vector2 control, Vector2 b, float t)
{
    const float inv = 1.0f - t;
    return Vector2{
        inv * inv * a.x + 2.0f * inv * t * control.x + t * t * b.x,
        inv * inv * a.y + 2.0f * inv * t * control.y + t * t * b.y
    };
}

inline void drawCurve(Vector2 a, Vector2 b, Color color)
{
    const Vector2 control{(a.x + b.x) * 0.5f + 26.0f, (a.y + b.y) * 0.5f - 42.0f};
    Vector2 previous = a;
    for (int i = 1; i <= 20; ++i)
    {
        const float t = static_cast<float>(i) / 20.0f;
        const Vector2 point = quadraticPoint(a, control, b, t);
        DrawLineEx(previous, point, 2.0f, color);
        previous = point;
    }
}

inline Vector2 pointOnCurve(Vector2 a, Vector2 b, float t)
{
    const Vector2 control{(a.x + b.x) * 0.5f + 26.0f, (a.y + b.y) * 0.5f - 42.0f};
    return quadraticPoint(a, control, b, t);
}
}
