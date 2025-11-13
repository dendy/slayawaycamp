
#pragma once

// #define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#include "Common.hpp"
#endif




#ifdef ENABLE_DEBUG
static constexpr int kDebugMoveId = 6;
static constexpr Dir kDebugMoveDir = Dir::Down;
#endif
