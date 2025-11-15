
#pragma once

// #define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#include "Common.hpp"
#endif




#ifdef ENABLE_DEBUG
static constexpr int kDebugMoveId = 6;
static constexpr Dir kDebugMoveDir = Dir::Down;

static constexpr std::string_view kDebugExpectedSteps = "ldururrdruruldurdldldllururdlur";
static constexpr bool kDebugOnlyExpectedSteps = false;
#endif


static constexpr int kShowStepsVerbosity = 2;
static constexpr int kShowStepsCount = -1;
