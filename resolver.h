#pragma once

// Forward declarations - don't redefine existing types
class Player;
class LagRecord;
class AimPlayer;
struct MoveData_t;
class Client;

// Only include essential system headers here
#include <algorithm>
#include <vector>
#include <array>
#include <cmath>
#include <string>
#include <cfloat>
#include <map>
#include <memory>

// Use existing enums from lagrecord.h - don't redefine them
// RESOLVE_* enums are already defined in lagrecord.h
// Modes class is already defined in lagrecord.h
// Add missing constants that might not be defined elsewhere


// Miss counter constants - use different names to avoid conflicts
enum ResolverMissCounters {
    RMISS_STAND = 0,
    RMISS_STAND_NO_DATA,
    RMISS_UPDATE_PRED,
    RMISS_NOUPDATE,
    RMISS_STAND_BALANCE,
    RMISS_STAND_BALANCE2,
    RMISS_STAND_DISTORTION,
    RMISS_LASTMOVE,
    RMISS_AIR,
    RMISS_MAX
};

// Helper functions
inline float to_time(int ticks) {
    return game::TICKS_TO_TIME(ticks);
}

inline int to_ticks(float time) {
    return game::TIME_TO_TICKS(time);
}

// Globals structure
struct globals_t {
    float tickinterval = 0.015625f; // 64 tick server interval (1.0f / 64.0f)

    bool is_valid_tick(int tick) {
        return tick > 0 && tick < 2048; // Example validation
    }

    bool is_valid_time(float time) {
        return time > 0.0f && time < 60.0f; // Example validation
    }
};

extern globals_t* globals;

// Forward declarations and structures for lag compensation
struct c_lagcompensation {
    float lby_timer;
    float co_efficiency;
    bool valid_timer_layer1;

    __forceinline c_lagcompensation() {
        lby_timer = 0.0f;
        co_efficiency = 0.0f;
        valid_timer_layer1 = false;
    }
};

// Global context structure
struct ctx {
    static constexpr int clock_desync = 2; // Example value, adjust as needed
};

// Animation state helper
struct c_animation_state {
    static bool is_unturned(float timer, C_AnimationLayer* layers, C_AnimationLayer* interpolated_layers) {
        // Implementation for checking if animation is unturned
        if (!layers || !interpolated_layers) return false;
        // Add your specific logic here - this is a placeholder
        return timer > 0.0f && layers[6].m_weight > 0.0f;
    }
};

// Function to check timer layer
inline bool correct_timer_layer1(c_lagcompensation* entry) {
    if (!entry) return false;
    return entry->valid_timer_layer1;
}

// Use a different name to avoid conflicts with existing AdaptiveAngle
struct ResolverAdaptiveAngle {
    float m_yaw = 0.f;
    float m_dist = 0.f;

    ResolverAdaptiveAngle() = default;
    ResolverAdaptiveAngle(float yaw) : m_yaw(yaw), m_dist(0.f) {}
};

struct ResolverLBYTracking_t {
    float m_last_lby_angles[3];
    float m_first_lby_after_opposite;
    float m_opposite_vel_start_time;
    float m_fakewalk_start_time;
    float m_last_update_time;
    float m_last_sim_time;
    bool m_has_constant_lby;
    bool m_manual_change_detected;
    bool m_is_fakewalking;
    bool m_confirmed_fakewalk;
    int m_lby_update_count;
    int m_lagcomp_issues;

    __forceinline ResolverLBYTracking_t() {
        for (int i = 0; i < 3; ++i)
            m_last_lby_angles[i] = 0.f;
        m_first_lby_after_opposite = 0.f;
        m_opposite_vel_start_time = 0.f;
        m_fakewalk_start_time = 0.f;
        m_last_update_time = 0.f;
        m_last_sim_time = 0.f;
        m_has_constant_lby = false;
        m_manual_change_detected = false;
        m_is_fakewalking = false;
        m_confirmed_fakewalk = false;
        m_lby_update_count = 0;
        m_lagcomp_issues = 0;
    }
};

struct ResolverOverrideData {
    float m_yaw;
    vec3_t m_start, m_end;
    Player* m_player;

    __forceinline ResolverOverrideData() {
        m_yaw = 0.f;
        m_start = vec3_t(0, 0, 0);
        m_end = vec3_t(0, 0, 0);
        m_player = nullptr;
    }
};

// Player entry getter macro
#define GET_PLAYER_ENTRY(player) (&g_resolver.m_lagcomp_entries[player->index() - 1])

class Resolver {
public:
    bool m_override, m_override_update;
    ang_t m_override_angle;
    ResolverOverrideData m_override_data;

    std::array<ResolverLBYTracking_t, 64> m_lby_tracking;
    std::array<c_lagcompensation, 64> m_lagcomp_entries;

public:
    // Main resolver methods
    void ResolveAngles(AimPlayer* data, LagRecord* current, LagRecord* previous);
    void ResolveStand(AimPlayer* data, LagRecord* current, LagRecord* previous);
    void ResolveMove(AimPlayer* data, LagRecord* current, LagRecord* previous);
    void ResolveAir(AimPlayer* data, LagRecord* current, LagRecord* previous);

    // Helper methods
    void HandleModes(AimPlayer* data, LagRecord* current, LagRecord* previous);
    void HandlePredUpdate(AimPlayer* data, LagRecord* current, LagRecord* previous);
    void MatchShot(AimPlayer* data, LagRecord* current, LagRecord* previous);
    void UpdateLBYTracking(AimPlayer* data, LagRecord* current);
    void UpdateLagCompensation(Player* player);
    void Override();

    // Utility methods
    float AutoDirection(Player* player, std::vector<ResolverAdaptiveAngle> angles = {});
    float FindBestYaw(LagRecord* record, MoveData_t* move_data);
};

extern Resolver g_resolver;

// Lagcomp globals - declared in lagcomp.h
// extern LagCompensation g_lagcomp;

// FLT_MAX definition if not available
#ifndef FLT_MAX
#define FLT_MAX 3.402823466e+38F
#endif
