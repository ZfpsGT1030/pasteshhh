#pragma once

struct AnimationBackup_t {
	vec3_t           m_origin, m_abs_origin;
	vec3_t           m_velocity, m_abs_velocity;
	int              m_flags, m_eflags;
	float            m_duck, m_body;
	C_AnimationLayer m_layers[13];
};

enum {
	MISS_STAND,
	MISS_STAND_BALANCE,
	MISS_STAND_BALANCE2,
	MISS_STAND_NO_DATA,
	MISS_STAND_DISTORTION,
	MISS_NOUPDATE,
	MISS_UPDATE_PRED,
	MISS_AIR,
	MISS_LASTMOVE,
	MISS_MAX
};

struct MoveData_t {
	float m_body;
	float m_body_delta;
	int   m_updates;
	float m_anim_time;

	__forceinline MoveData_t() {
		Reset();
	}

	__forceinline void Reset() {
		m_body = 0.f;
		m_body_delta = 0.f;
		m_updates = 0;
		m_anim_time = -1.f;
	}

	__forceinline void Store(LagRecord* current) {
		m_body = current->m_body;
		m_body_delta = 0.f;
		m_updates = 0;
		m_anim_time = current->m_anim_time;
	}
};

struct FlickData_t {
	int   m_updates;
	int   m_foot_side;
	float m_next_body_update;
	bool  m_distortion;
	bool  m_should_have_updated;

	__forceinline FlickData_t() {
		Reset();
	}

	__forceinline void Reset() {
		m_updates = 0;
		m_foot_side = 0;
		m_next_body_update = FLT_MAX;
		m_distortion = false;
		m_should_have_updated = false;
	}

	__forceinline void OnMove(float time) {
		m_updates = 0;
		m_foot_side = 0;
		m_next_body_update = time + 0.22f;
		m_distortion = false;
		m_should_have_updated = false;
	}
};

struct FootData_t {
	float m_move_yaw;
	float m_foot_yaw;
	C_AnimationLayer m_layers[13];

	FootData_t() {
		m_move_yaw = 0.f;
		m_foot_yaw = 0.f;
	}

	void Store(CCSGOPlayerAnimState* state) {
		m_foot_yaw = state->m_abs_yaw;
		m_move_yaw = state->m_move_yaw;
		state->m_player->GetAnimLayers(m_layers);
	}

	void Set(CCSGOPlayerAnimState* state) {
		state->m_abs_yaw = m_foot_yaw;
		state->m_move_yaw = m_move_yaw;
	}
};

class AimPlayer {
public:
	// essential data.
	Player* m_player;
	CCSGOPlayerAnimState* m_state;

	MoveData_t m_move_data;
	FlickData_t m_update_data;
	float	   m_spawn;
	float      m_last_shift_time;
	float      m_max_speed;
	vec3_t     m_anim_velocity;
	float      m_last_body; // Add missing m_last_body variable

	std::pair<LagRecord, LagRecord> m_predicted;
	std::deque< LagRecord >  m_records;

	int m_misses[MISS_MAX];

	FootData_t m_foot_data[3];

	bool m_white_listed;
public:
	void SyncPlayerVelocity(LagRecord* current, LagRecord* previous);
	void HandleSafePoints(LagRecord* current, LagRecord* previous);
	void UpdateAnimations(LagRecord* current, LagRecord* previous);
	void OnNetUpdate(Player* player);
public:

	__forceinline void Reset() {
		m_player = nullptr;
		m_state = nullptr;

		m_predicted.first = { };
		m_predicted.second = { };

		m_spawn = 0.f;
		m_last_shift_time = 0.f;
		m_max_speed = 260.f;
		m_anim_velocity = { };
		m_last_body = 0.f;

		m_move_data.Reset();
		m_update_data.Reset();
		m_records.clear();

		// CRASH FIX: Ensure all miss counters are initialized
		std::memset(m_misses, 0, sizeof(m_misses));
		m_white_listed = false;
	}
};


class PointData {
public:
	vec3_t m_point;
	float  m_damage;
	int    m_hitbox;
	int    m_hitgroup;
	float  m_safety;
	float  m_hitchance;

	LagRecord* m_record;
	mstudiobbox_t* m_bbox;

	PointData() {
		m_point = { 0, 0, 0 };
		m_damage = 0;
		m_hitbox = -1;
		m_hitgroup = -1;
		m_safety = 0.f;
		m_hitchance = 0.f;
		m_record = nullptr;
		m_bbox = nullptr;
	}

	PointData(const vec3_t& point, LagRecord* record, mstudiobbox_t* bbox) {
		m_point = point;
		m_record = record;
		m_bbox = bbox;
	}
};

class TargetData {
public:
	Player* m_player;
	int           m_index;
	int           m_best_damage;
	int           m_best_hitchance;
	std::vector<LagRecord*> m_records;
	std::vector<PointData> m_points;

	TargetData() {
		m_player = nullptr;
		m_index = -1;
		m_best_damage = 0;
		m_best_hitchance = 0;
		m_records.clear();
		m_points.clear();
	}
};

class Aimbot {
public:
	// binds.
	bool m_fake_latency;
	bool m_damage_override;

	std::array< AimPlayer, 64 > m_players;
	std::array<SpreadSeeds_t, 256> m_spread_seeds;
public:
	ang_t       m_angle;
	PenInput_t  m_input;
	PenOutput_t m_output;

	int   m_minimum_damage;
	float m_hitchance;

	std::vector<int> m_hitboxes;
	std::vector<TargetData> m_targets;

	std::array<float, HITGROUP_GEAR> m_hitgroup_damage;
public:
	bool IsValidTarget(Player* player);

	void SetupSettings();
	void SetupHitboxes();

	void Think();
	void FindTargets();
	void CreatePoints();
	bool ScanAimPoints(TargetData* target, bool refine = false);
	PointData* FindBestPoint(TargetData* target);

	void ComputeHitchanceSeeds();
	float PointHitchance(PointData* point, const ang_t& angle, const size_t& max_seeds) const;
	bool  CheckHitchance(PointData* point, const ang_t& angle, const float& chance, const size_t& max_seeds);
	size_t CheckIntersection(PointData* point);
	int GetMinimumIntersections(LagRecord* record);
	
	// Add missing function declarations
	float GetHitchance(AimPlayer* data, LagRecord* record, const vec3_t& aim_point, float distance);
	float CalculateHitchance(AimPlayer* data, LagRecord* record);
};

extern Aimbot g_aimbot;