#pragma once
#include <array>

// Discrete actions the car can take
// 0=Left, 1=Right, 2=Accelerate, 3=Brake, 4=None
enum class Action { Left = 0, Right = 1, Accelerate = 2, Brake = 3, None = 4 };

// Sensor + physics snapshot sent to Python for inference
struct CarState {
    float distLeft;        // sensor at -60 deg from heading
    float distFrontLeft;   // sensor at -30 deg
    float distFront;       // sensor at   0 deg
    float distFrontRight;  // sensor at +30 deg
    float distRight;       // sensor at +60 deg
    float velocity;        // current car speed px/s
};

// Reward shaping: reward forward motion, heavily penalise crashes
inline float computeReward(const CarState& s, bool crashed, float speedDelta) {
    if (crashed) return -500.f;  // Make crashing very bad so it learns to survive
    
    float r = speedDelta * 3.0f;          // Reward for distance covered
    
    // Tiny proximity penalty so it prefers the center of the track
    // (reduced from -3 to -0.1 to avoid dominating the reward loop over time)
    if (s.distFront      < 40.f) r -= 0.1f;
    if (s.distLeft       < 25.f) r -= 0.05f;
    if (s.distRight      < 25.f) r -= 0.05f;
    if (s.distFrontLeft  < 25.f) r -= 0.05f;
    if (s.distFrontRight < 25.f) r -= 0.05f;
    
    return r;
}
