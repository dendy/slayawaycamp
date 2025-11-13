
#pragma once

// #define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#include "Common.hpp"
#endif




#ifdef ENABLE_DEBUG
static constexpr int kDebugMoveId = 6;
static constexpr Dir kDebugMoveDir = Dir::Down;

static const std::string_view kDebugExpectedSteps = "llur";
// static const std::string_view kDebugExpectedSteps = "llurrdurullululullu";
#endif


static constexpr int kShowStepsVerbosity = 2;
static constexpr int kShowStepsCount = -1;
