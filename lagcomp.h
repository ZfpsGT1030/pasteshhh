#pragma once

#include <map>
#include <initializer_list>
#include <cstdint>

// Forward declarations
class AimPlayer;
class Player;
class LagRecord;
struct C_AnimationLayer;
struct CCSGOPlayerAnimState;
struct vec3_t;

// Add missing flag constant
#ifndef FL_ONGROUND
#define FL_ONGROUND (1<<0)
#endif

// Your exact adaptive yaw resolver structures
struct c_pvs_map {
	int flags;
	uint32_t ftm;
	// Add other members as needed
};

struct c_cs_player {
	int index() { return 0; } // Placeholder - implement actual index function
	void* pvs_instance() { return nullptr; } // Placeholder - implement actual pvs_instance function
};

// FFM flags for your adaptive yaw resolver
#define FFM_IK_PRECISE 0x1
#define FFM_MP_DEVONLY 0x2
#define FFM_LOW_IK 0x4
#define FFM_ABNORMAL 0x8
#define FFM_AS_RAW 0x10

// Your conditional map implementation
template<typename T>
class conditional_map {
public:
	conditional_map(std::initializer_list<std::pair<int, T>> init) {
		for (const auto& pair : init) {
			m_map[pair.first] = pair.second;
		}
	}

	T operator()(int flags) const {
		for (const auto& pair : m_map) {
			if (pair.first == -1) return pair.second; // Default value
			if ((flags & pair.first) == pair.first) return pair.second;
		}
		return T{};
	}

private:
	std::map<int, T> m_map;
};

class LagCompensation {
public:
	enum LagType : size_t {
		INVALID = 0,
		CONSTANT,
		ADAPTIVE,
		RANDOM,
	};

private:
	// animfix backup stuff.
	float  m_curtime;
	float  m_frametime;
	vec3_t m_origin;
	vec3_t m_abs_origin;
	vec3_t m_velocity;
	vec3_t m_abs_velocity;
	float  m_duck_amount;
	float  m_duck_speed;
	int    m_flags;
	C_AnimationLayer m_layers[13];
	float            m_poses[20];
	CCSGOPlayerAnimState m_state;

public:
	void PlayerMove(LagRecord* record);
	LagRecord* StartPrediction(AimPlayer* data);
	void UpdatePredictedAnimations(AimPlayer* data, LagRecord* predicted, LagRecord* previous, int update_ticks);
	void Store(Player* player);
	void Restore(Player* player);
};

// Your exact adaptive yaw resolver namespace

// Global lagcomp instance
extern LagCompensation g_lagcomp;