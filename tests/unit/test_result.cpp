#include "core/error/result.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace northern::core;

TEST(ResultTest, SuccessWithValue) {
    Result<int> res(42);
    EXPECT_TRUE(res.isOk());
    EXPECT_FALSE(res.isErr());
    EXPECT_TRUE(static_cast<bool>(res));
    EXPECT_EQ(res.value(), 42);
    EXPECT_EQ(res.valueOr(0), 42);
}

TEST(ResultTest, FailureWithError) {
    Error err(ErrorCode::InvalidArgument, "Invalid parameter");
    Result<int> res(err);
    EXPECT_FALSE(res.isOk());
    EXPECT_TRUE(res.isErr());
    EXPECT_FALSE(static_cast<bool>(res));
    EXPECT_EQ(res.error().code, ErrorCode::InvalidArgument);
    EXPECT_EQ(res.valueOr(100), 100);
}

TEST(ResultTest, MapTransformation) {
    Result<int> res(21);
    auto mapped = res.map([](int val) { return val * 2; });
    EXPECT_TRUE(mapped.isOk());
    EXPECT_EQ(mapped.value(), 42);

    Result<int> failed(Error(ErrorCode::NotFound, "Missing"));
    auto failedMapped = failed.map([](int val) { return val * 2; });
    EXPECT_TRUE(failedMapped.isErr());
    EXPECT_EQ(failedMapped.error().code, ErrorCode::NotFound);
}

TEST(ResultTest, FlatMapMonadicChaining) {
    auto parseEven = [](int val) -> Result<std::string> {
        if (val % 2 == 0) {
            return std::to_string(val);
        }
        return Error(ErrorCode::InvalidArgument, "Not even");
    };

    Result<int> valid(4);
    auto chainedValid = valid.flatMap(parseEven);
    EXPECT_TRUE(chainedValid.isOk());
    EXPECT_EQ(chainedValid.value(), "4");

    Result<int> invalid(5);
    auto chainedInvalid = invalid.flatMap(parseEven);
    EXPECT_TRUE(chainedInvalid.isErr());
    EXPECT_EQ(chainedInvalid.error().code, ErrorCode::InvalidArgument);
}

TEST(ResultTest, VoidResultHandling) {
    auto okRes = Result<void>::ok();
    EXPECT_TRUE(okRes.isOk());
    EXPECT_FALSE(okRes.isErr());

    Result<void> errRes(Error(ErrorCode::InternalError, "Failed"));
    EXPECT_FALSE(errRes.isOk());
    EXPECT_TRUE(errRes.isErr());
    EXPECT_EQ(errRes.error().code, ErrorCode::InternalError);
}
