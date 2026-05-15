#pragma once

namespace memory_playground
{
constexpr int kWindowWidth = 1360;
constexpr int kMinWindowWidth = 1280;

constexpr float kUiOuterMargin = 30.0f;
constexpr float kUiTopY = 74.0f;
constexpr float kUiPanelGap = 14.0f;
constexpr float kUiFooterHeight = 44.0f;

constexpr float kUiMetricsHeight = 104.0f;
constexpr float kUiGraphHeight = 104.0f;
constexpr float kUiHeaderHeight = 42.0f;
constexpr int kUiSettingsRowCount = 5;
constexpr float kUiSettingsTopOffset = 52.0f;
constexpr float kUiSettingsRowHeight = 30.0f;
constexpr float kUiSettingsRowContentHeight = 22.0f;
constexpr float kUiSettingsBottomPad = 18.0f;
constexpr float kUiSettingsHeight =
    kUiSettingsTopOffset +
    kUiSettingsRowHeight * static_cast<float>(kUiSettingsRowCount - 1) +
    kUiSettingsRowContentHeight +
    kUiSettingsBottomPad;
constexpr float kUiChallengeRowHeight = 58.0f;
constexpr float kUiChallengeRowGap = 8.0f;
constexpr int kUiDefaultChallengeCount = 4;
constexpr float kUiLearnHeight = 150.0f;

constexpr float kUiChallengesHeight =
    kUiHeaderHeight +
    kUiChallengeRowHeight * static_cast<float>(kUiDefaultChallengeCount) +
    kUiChallengeRowGap * static_cast<float>(kUiDefaultChallengeCount - 1) +
    16.0f;

constexpr float kUiRightColumnContentHeight =
    kUiMetricsHeight + kUiPanelGap +
    kUiGraphHeight + kUiPanelGap +
    kUiSettingsHeight + kUiPanelGap +
    kUiChallengesHeight + kUiPanelGap +
    kUiLearnHeight;

constexpr int kWindowHeight = static_cast<int>(
    kUiTopY +
    kUiRightColumnContentHeight +
    kUiPanelGap +
    kUiFooterHeight +
    kUiOuterMargin +
    0.5f
);

constexpr int kMinWindowHeight = kWindowHeight;
}
