#include "core/animation/animation_manager.hpp"
#include "core/animation/easing.hpp"
#include "core/animation/property_animation.hpp"
#include "core/animation/spring.hpp"
#include <gtest/gtest.h>

using namespace northern::core;
using namespace northern::core::animation;

TEST(AnimationTest, EasingCurvesBoundaries) {
    std::vector<EasingCurve> curves = {
        EasingCurve::Linear,     EasingCurve::InQuad,  EasingCurve::OutQuad,
        EasingCurve::InOutQuad,  EasingCurve::InCubic, EasingCurve::OutCubic,
        EasingCurve::InOutCubic, EasingCurve::OutExpo, EasingCurve::InOutExpo,
        EasingCurve::OutElastic,
    };

    for (auto c : curves) {
        EXPECT_NEAR(evaluateEasing(c, 0.0f), 0.0f, 0.001f);
        EXPECT_NEAR(evaluateEasing(c, 1.0f), 1.0f, 0.001f);
    }
}

TEST(AnimationTest, CubicBezierParametricSolver) {
    // Ease-in-out standard approximation: (0.42, 0.0, 0.58, 1.0)
    EXPECT_NEAR(cubicBezier(0.42f, 0.0f, 0.58f, 1.0f, 0.0f), 0.0f, 0.001f);
    EXPECT_NEAR(cubicBezier(0.42f, 0.0f, 0.58f, 1.0f, 1.0f), 1.0f, 0.001f);
    EXPECT_NEAR(cubicBezier(0.42f, 0.0f, 0.58f, 1.0f, 0.5f), 0.5f, 0.01f);
}

TEST(AnimationTest, SpringSettlingAndRest) {
    Spring spring{0.0f, SpringConfig::Default()};
    spring.setTarget(100.0f);
    EXPECT_FALSE(spring.isAtRest());

    // Advance in 16ms steps
    for (int i = 0; i < 200; ++i) {
        spring.update(0.016f);
        if (spring.isAtRest()) {
            break;
        }
    }

    EXPECT_TRUE(spring.isAtRest());
    EXPECT_NEAR(spring.position(), 100.0f, 0.001f);
    EXPECT_NEAR(spring.velocity(), 0.0f, 0.001f);
}

TEST(AnimationTest, PropertyAnimationFloatEasing) {
    PropertyAnimation<float> anim;
    int updateCount = 0;
    bool finished = false;

    anim.onUpdate.connect([&](const float&) { ++updateCount; });
    anim.onFinished.connect([&]() { finished = true; });

    anim.start(0.0f, 10.0f, 0.1f, EasingCurve::Linear);
    EXPECT_TRUE(anim.isRunning());
    EXPECT_FALSE(anim.isFinished());

    anim.update(0.05f);
    EXPECT_NEAR(anim.currentValue(), 5.0f, 0.05f);
    EXPECT_GE(updateCount, 1);
    EXPECT_FALSE(finished);

    anim.update(0.06f);
    EXPECT_NEAR(anim.currentValue(), 10.0f, 0.001f);
    EXPECT_TRUE(anim.isFinished());
    EXPECT_TRUE(finished);
}

TEST(AnimationTest, PropertyAnimationColor) {
    PropertyAnimation<Color> anim;
    Color c0 = Color::fromRgba8(0, 0, 0, 255);
    Color c1 = Color::fromRgba8(200, 100, 50, 255);

    anim.start(c0, c1, 0.2f, EasingCurve::Linear);
    anim.update(0.1f);

    EXPECT_NEAR(anim.currentValue().r, c1.r * 0.5f, 0.02f);
    EXPECT_NEAR(anim.currentValue().g, c1.g * 0.5f, 0.02f);
    EXPECT_NEAR(anim.currentValue().b, c1.b * 0.5f, 0.02f);
}

TEST(AnimationTest, AnimationManagerTickingAndPruning) {
    AnimationManager mgr;
    EXPECT_FALSE(mgr.hasActiveAnimations());
    EXPECT_EQ(mgr.activeAnimationCount(), 0);

    float val = 0.0f;
    uint32_t id1 = mgr.addAnimation([&val](float dt) {
        val += dt;
        return val < 0.1f;
    });
    EXPECT_NE(id1, 0);
    EXPECT_TRUE(mgr.hasActiveAnimations());
    EXPECT_EQ(mgr.activeAnimationCount(), 1);

    mgr.advance(0.05f);
    EXPECT_NEAR(val, 0.05f, 0.001f);
    EXPECT_TRUE(mgr.hasActiveAnimations());

    // Second tick finishes animation
    mgr.advance(0.06f);
    EXPECT_GE(val, 0.1f);
    EXPECT_FALSE(mgr.hasActiveAnimations());
    EXPECT_EQ(mgr.activeAnimationCount(), 0);
}
