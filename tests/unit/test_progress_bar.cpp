#include "widgets/progress_bar.hpp"
#include <gtest/gtest.h>

using namespace northern;
using namespace northern::widgets;

TEST(ProgressBarTest, InitialStateAndDefaults) {
    ProgressBar bar;
    EXPECT_FLOAT_EQ(bar.progress(), 0.0f);
    EXPECT_FALSE(bar.isIndeterminate());
    EXPECT_FLOAT_EQ(bar.barHeight(), 4.0f);
}

TEST(ProgressBarTest, ProgressClamping) {
    ProgressBar bar;
    bar.setProgress(0.5f);
    EXPECT_FLOAT_EQ(bar.progress(), 0.5f);

    bar.setProgress(-0.25f);
    EXPECT_FLOAT_EQ(bar.progress(), 0.0f);

    bar.setProgress(1.5f);
    EXPECT_FLOAT_EQ(bar.progress(), 1.0f);
}

TEST(ProgressBarTest, IndeterminateModeAndAnimation) {
    ProgressBar bar;
    bar.setIndeterminate(true);
    EXPECT_TRUE(bar.isIndeterminate());

    float initialOffset = bar.indeterminateOffset();
    bar.advanceAnimation(0.5f);
    EXPECT_NE(bar.indeterminateOffset(), initialOffset);

    // Full cycle wraps around [0.0, 1.0]
    bar.advanceAnimation(5.0f);
    EXPECT_GE(bar.indeterminateOffset(), 0.0f);
    EXPECT_LE(bar.indeterminateOffset(), 1.0f);
}
