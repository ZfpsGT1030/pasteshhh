#pragma once

// pre-declare.
class LagRecord;

class BackupRecord {
public:
	BoneArray  m_bones[128];
	vec3_t     m_origin, m_abs_origin;
	vec3_t     m_mins;
	vec3_t     m_maxs;
	ang_t      m_abs_ang;

public:
	__forceinline void store(Player* player) {
		m_origin = player->m_vecOrigin();
		m_mins = player->m_vecMins();
		m_maxs = player->m_vecMaxs();
		m_abs_origin = player->GetAbsOrigin();
		m_abs_ang = player->GetAbsAngles();

		player->GetBones(m_bones);
	}

	__forceinline void restore(Player* player) {
		player->m_vecMins() = m_mins;
		player->m_vecMaxs() = m_maxs;
		player->SetAbsAngles(m_abs_ang);
		player->SetAbsOrigin(m_abs_origin);
		player->m_vecOrigin() = m_origin;

		player->SetBones(m_bones);
	}
};

enum {
	LC_LOST_TRACK,
	LC_TIME_DECREMENT,
	LC_FAKE_UPDATE
};

class Modes {
public:
	enum {
		RESOLVE_LASTMOVE = 14,  // Use the RESOLVE_LASTMOVE value
		RESOLVE_FAKE = 15       // Add RESOLVE_FAKE mode
	};
};

enum {
	RESOLVE_NONE,
	RESOLVE_MOVE,
	RESOLVE_STAND,
	RESOLVE_STAND_BALANCE,
	RESOLVE_STAND_BALANCE2,
	RESOLVE_STAND_NO_DATA,
	RESOLVE_DISTORTION,
	RESOLVE_NOUPDATE,
	RESOLVE_PREUPDATE,
	RESOLVE_UPDATE,
	RESOLVE_UPDATE_PRED,
	RESOLVE_AIR,
	RESOLVE_AIR_UPDATE,
	RESOLVE_OVERRIDE,
	RESOLVE_LASTMOVE,
	RESOLVE_FAKE,
	// we use this to mark as low resolve chance.
	RESOLVE_PREDICTED
};

enum {
	RESOLVE_CONFIDENCE_LOW,
	RESOLVE_CONFIDENCE_MEDIUM,
	RESOLVE_CONFIDENCE_HIGH,
	RESOLVE_CONFIDENCE_VERY_HIGH,
	RESOLVE_CONFIDENCE_OVERRIDE,
};

struct SafeData_t {
	BoneArray m_bones[128];
	float     m_yaw;

	SafeData_t() {
		m_yaw = 0.f;
	}
};

class LagRecord {
public:
	// data.
	Player* m_player;
	EHANDLE m_weapon_handle;
	float   m_time, m_anim_time;
	float   m_duck, m_duck_speed;

	float   m_body;

	int     m_flags, m_anim_flags;
	int     m_move_state, m_move_type;

	ang_t   m_angles, m_abs_angles;

	vec3_t  m_origin, m_abs_origin;
	vec3_t  m_velocity, m_abs_velocity;

	vec3_t  m_mins, m_maxs;
	float   m_surface_friction; // For enhanced movement prediction

	C_AnimationLayer m_layers[13];
	float m_poses[19];

	BoneArray  m_bones[128];
	SafeData_t m_safe_bones[36];
	size_t     m_safe_count;

	int     m_server_tick;
	int     m_choke;

	bool    m_lagcomp[3]; // FIX: Increased size to accommodate LC_FAKE_UPDATE
	int     m_resolver_mode;
	std::string m_resolver_mode_name;
	bool    m_missed;
	bool    m_predicted;
	bool    m_fake_walk;
	bool    m_lastmove_oppsite_to_move_dir;
	ang_t   m_eye_angles;
	int     m_mode;
	bool    m_shot; // FIX: Add missing m_shot member

	__forceinline LagRecord() {
		m_player = nullptr;
		m_weapon_handle = NULL;
		m_safe_count = 0;
		m_server_tick = -1;
		m_choke = 1;
		m_surface_friction = 1.0f; // Default surface friction

		m_resolver_mode = RESOLVE_NONE;
		m_resolver_mode_name = "";
		m_missed = false;
		m_predicted = false;
		m_fake_walk = false;
		m_lastmove_oppsite_to_move_dir = false;
		m_eye_angles = ang_t(0, 0, 0);
		m_mode = 0;
		m_shot = false; // FIX: Initialize m_shot

		for (bool& lc : m_lagcomp)
			lc = false;
	}
	
	// CRASH FIX: Copy constructor that doesn't copy the player pointer
	__forceinline LagRecord(const LagRecord& other) {
		// Copy all data except the player pointer
		m_player = nullptr; // Never copy the player pointer!
		m_weapon_handle = other.m_weapon_handle;
		m_time = other.m_time;
		m_anim_time = other.m_anim_time;
		m_duck = other.m_duck;
		m_duck_speed = other.m_duck_speed;
		m_body = other.m_body;
		m_flags = other.m_flags;
		m_anim_flags = other.m_anim_flags;
		m_move_state = other.m_move_state;
		m_move_type = other.m_move_type;
		m_angles = other.m_angles;
		m_abs_angles = other.m_abs_angles;
		m_origin = other.m_origin;
		m_abs_origin = other.m_abs_origin;
		m_velocity = other.m_velocity;
		m_abs_velocity = other.m_abs_velocity;
		m_mins = other.m_mins;
		m_maxs = other.m_maxs;
		m_surface_friction = other.m_surface_friction;
		m_safe_count = other.m_safe_count;
		m_server_tick = other.m_server_tick;
		m_choke = other.m_choke;
		m_resolver_mode = other.m_resolver_mode;
		m_resolver_mode_name = other.m_resolver_mode_name;
		m_missed = other.m_missed;
		m_predicted = other.m_predicted;
		m_fake_walk = other.m_fake_walk;
		m_lastmove_oppsite_to_move_dir = other.m_lastmove_oppsite_to_move_dir;
		m_eye_angles = other.m_eye_angles;
		m_mode = other.m_mode;
		m_shot = other.m_shot; // FIX: Copy m_shot
		
		// Copy arrays
		std::memcpy(m_layers, other.m_layers, sizeof(m_layers));
		std::memcpy(m_poses, other.m_poses, sizeof(m_poses));
		std::memcpy(m_bones, other.m_bones, sizeof(m_bones));
		std::memcpy(m_safe_bones, other.m_safe_bones, sizeof(m_safe_bones));
		std::memcpy(m_lagcomp, other.m_lagcomp, sizeof(m_lagcomp));
	}

	__forceinline ~LagRecord() {
		m_player = nullptr;
		m_safe_count = 0;
	}

public:
	__forceinline void Store(Player* player) {
		m_player = player;
		m_weapon_handle = player->m_hActiveWeapon();

		m_time = m_anim_time = player->m_flSimulationTime();

		m_duck = player->m_flDuckAmount();
		m_duck_speed = player->m_flDuckSpeed();

		m_body = player->m_flLowerBodyYawTarget();

		m_flags = m_anim_flags = player->m_fFlags();

		m_angles = player->m_angEyeAngles();

		m_origin = m_abs_origin = player->m_vecOrigin();
		m_velocity = m_abs_velocity = player->m_vecVelocity();

		m_move_state = player->m_iMoveState();
		m_move_type = player->m_MoveType();

		player->GetAnimLayers(m_layers);

		m_server_tick = g_csgo.m_cl->m_server_tick;

		// FIX: Detect if player was shooting
		Weapon* weapon = player->GetActiveWeapon();
		if (weapon && weapon->m_flNextPrimaryAttack() <= player->m_flSimulationTime()) {
			m_shot = true;
		} else {
			m_shot = false;
		}
	}

	__forceinline void UpdateBounds() {
		m_mins = m_player->m_vecMins();
		m_maxs = m_player->m_vecMaxs();
	}

	__forceinline void SetData() {
		m_player->SetAbsOrigin(m_origin);
		m_player->SetAbsAngles(m_abs_angles);

		m_player->m_vecMins() = m_mins;
		m_player->m_vecMaxs() = m_maxs;

		m_player->SetBones(m_bones);
	}

	__forceinline void SetData(Player* player) {
		player->SetAbsOrigin(m_origin);
		player->SetAbsAngles(m_abs_angles);

		player->m_vecMins() = m_mins;
		player->m_vecMaxs() = m_maxs;

		player->SetBones(m_bones);
	}

	__forceinline bool ValidTime() const {
		// lagcompensation is turned off.
		if (g_csgo.cl_lagcompensation->GetInt() == 0)
			return false;

		int dead_time = game::TICKS_TO_TIME(g_cl.m_arrival_tick) - g_csgo.sv_maxunlag->GetFloat();

		// time is out of bounds.
		if (m_time < dead_time)
			return false;

		// the server wont accept our command.
		if (m_time > game::TICKS_TO_TIME(g_cl.m_arrival_tick + g_csgo.sv_max_usercmd_future_ticks->GetInt()))
			return false;

		float curtime = g_cl.m_local && g_cl.m_local->alive() ? g_cl.m_curtime : g_csgo.m_globals->m_curtime;

		// correct is the amount of time we have to correct game time
		float correct = g_cl.m_lerp + g_cl.m_latency[INetChannel::FLOW_OUTGOING] + g_cl.m_latency[INetChannel::FLOW_INCOMING];

		// check bounds [0,sv_maxunlag]
		correct = std::clamp(correct, 0.0f, g_csgo.sv_maxunlag->GetFloat());

		// calculate difference between tick sent by player and our latency based tick
		const float deltaTime = correct - (curtime - m_time);

		return fabs(deltaTime) <= 0.2f;
	}

	__forceinline bool IsBodyUpdate() const {
		switch (m_resolver_mode) {
		case RESOLVE_UPDATE:
		case RESOLVE_UPDATE_PRED:
		case RESOLVE_AIR_UPDATE:
			return true;
		}

		return false;
	}

	__forceinline int ResolveChance() const {
		switch (m_resolver_mode) {
			// they aint choking shit.
		case RESOLVE_NONE:
			return RESOLVE_CONFIDENCE_VERY_HIGH;
			// they are moving :D
		case RESOLVE_MOVE:
			return RESOLVE_CONFIDENCE_HIGH;
			// we might have them resolved :D
		case RESOLVE_STAND:
		case RESOLVE_STAND_BALANCE:
			return RESOLVE_CONFIDENCE_MEDIUM;
			// they might be doing some goofy antiaim.
		case RESOLVE_STAND_BALANCE2:
			return RESOLVE_CONFIDENCE_LOW;
			// oh fuck no...
		case RESOLVE_STAND_NO_DATA:
		case RESOLVE_DISTORTION:
			return RESOLVE_CONFIDENCE_LOW;
			// they are not breaking lby :D
		case RESOLVE_NOUPDATE:
			return RESOLVE_CONFIDENCE_HIGH;
			// they might be doing funny stuff or they can be right on lastmove :D
		case RESOLVE_PREUPDATE:
			return RESOLVE_CONFIDENCE_MEDIUM;
			// they updated lby :O
		case RESOLVE_UPDATE:
			return RESOLVE_CONFIDENCE_VERY_HIGH;
			// predict that flick :sunglasses:
		case RESOLVE_UPDATE_PRED:
			return RESOLVE_CONFIDENCE_HIGH;
			// rabbit man very hard to hit :(
		case RESOLVE_AIR:
			return RESOLVE_CONFIDENCE_LOW;
		case RESOLVE_AIR_UPDATE:
			return RESOLVE_CONFIDENCE_HIGH;
			// welp its just luck now.
		case RESOLVE_PREDICTED:
			return RESOLVE_CONFIDENCE_LOW;
			// opposite velocity resolver
		case RESOLVE_LASTMOVE:
			return RESOLVE_CONFIDENCE_HIGH;
			// fake angle resolver
		case RESOLVE_FAKE:
			return RESOLVE_CONFIDENCE_MEDIUM;
		}

		// go on master. resolve them.
		return RESOLVE_CONFIDENCE_OVERRIDE;
	}

	__forceinline bool IsResolved() const {
		switch (m_resolver_mode) {
		case RESOLVE_NONE:
		case RESOLVE_MOVE:
		case RESOLVE_UPDATE:
		case RESOLVE_UPDATE_PRED:
		case RESOLVE_AIR_UPDATE:
		case RESOLVE_LASTMOVE:
		case RESOLVE_FAKE:
			return true;
		}

		return false;
	}
};