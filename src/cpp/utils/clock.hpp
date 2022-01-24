#pragma once

#include <gou/types.hpp>

#include <chrono>
using Clock = std::chrono::high_resolution_clock;
using DeltaTimeDuration = std::chrono::duration<DeltaTime>;
using ElapsedTime = long long;
using ElapsedTimeDuration = std::chrono::duration<ElapsedTime>;
