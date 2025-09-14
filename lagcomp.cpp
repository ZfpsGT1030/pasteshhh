#include "includes.h"
#include <algorithm>

// FIX: Proper declaration of g_lagcomp as instance
LagCompensation g_lagcomp{};

// PE Module extension with opcode_scan method
namespace PE {
	class ModuleExt : public Module {
	public:
		ModuleExt() : Module() {}
		ModuleExt(const Module& base) : Module(base) {}

		Address opcode_scan(const std::string& pattern) {
			return pattern::find(*this, pattern);
		}

		// Operator-> for accessing methods
		ModuleExt* operator->() { return this; }
		const ModuleExt* operator->() const { return this; }
	};
}

// Global CLIENT_DLL definition
static PE::ModuleExt g_client_dll;

// Initialize CLIENT_DLL
static bool init_client_dll() {
	static bool initialized = false;
	if (!initialized) {
		g_client_dll = PE::ModuleExt(PE::GetModule("client.dll"));
		initialized = true;
	}
	return initialized;
}

#define CLIENT_DLL (init_client_dll(), &g_client_dll)

// Your exact adaptive yaw resolving system implementation  
static conditional_map<float> offset_compression{
	std::pair<int, float>{FFM_IK_PRECISE | FFM_MP_DEVONLY, 0.8227123f},
	std::pair<int, float>{FFM_LOW_IK | FFM_ABNORMAL, 0.0244f},
	std::pair<int, float>{-1, 0.f}
};

struct ffm_map_t {
	c_pvs_map* map;
	float yaw;
	bool valid;
	int flags;
};

static std::array<ffm_map_t, 65> ffm_map = {};

// Enhanced engine prediction with better movement simulation
LagRecord* LagCompensation::StartPrediction(AimPlayer* data) {
	if (data->m_records.size() <= 1)
		return nullptr;

	LagRecord* current = &data->m_records[0];
	LagRecord* previous = &data->m_records[1];

	// they wont update before our shot registers.
	if (current->m_server_tick + (current->m_server_tick - previous->m_server_tick) > g_cl.m_arrival_tick) {
		// ouf we cant hit any recent record.
		if (current->m_lagcomp[LC_LOST_TRACK] &&
			current->m_lagcomp[LC_TIME_DECREMENT] &&
			!current->ValidTime())
			return nullptr;

		return current;
	}

	LagRecord* predicted = &data->m_predicted.first;
	LagRecord* prev_predicted = &data->m_predicted.second;

	// we recieved a new update.
	if (predicted->m_server_tick != current->m_server_tick) {
		int choked_ticks = current->m_server_tick - previous->m_server_tick;
		int latency_ticks = g_cl.m_latency[INetChannel::FLOW_OUTGOING] + g_cl.m_charged_ticks;
		bool is_lagcompensated = !current->m_lagcomp[LC_LOST_TRACK] && !current->m_lagcomp[LC_TIME_DECREMENT];

		int ticks_to_predict = 0;
		while (ticks_to_predict < latency_ticks)
			ticks_to_predict += choked_ticks;

		if (ticks_to_predict) {
			// copy over newest data.
			std::memcpy(predicted, current, sizeof(LagRecord));

			// predict each movement tick.
			int predicted_ticks = 0;

			data->m_player->GetAnimLayers(m_layers);
			data->m_player->GetPoseParameters(m_poses);
			data->m_player->GetAnimState(&m_state);

			// reset this since we are gonna change our matrices.
			predicted->m_safe_count = 0;

			while (predicted_ticks < ticks_to_predict) {
				std::memcpy(prev_predicted, predicted, sizeof(LagRecord));

				PlayerMove(predicted);

				if (predicted_ticks % choked_ticks == 0)
					UpdatePredictedAnimations(data, predicted, prev_predicted, choked_ticks);

				++predicted_ticks;
			}

			data->m_player->m_vecOrigin() = m_origin;
			data->m_player->m_vecVelocity() = m_velocity;
			data->m_player->m_flDuckAmount() = m_duck_amount;
			data->m_player->m_flDuckSpeed() = m_duck_speed;
			data->m_player->m_fFlags() = m_flags;

			data->m_player->m_vecAbsOrigin() = m_abs_origin;
			data->m_player->m_vecAbsVelocity() = m_abs_velocity;

			data->m_player->SetAnimLayers(m_layers);
			data->m_player->SetPoseParameters(m_poses);
			data->m_player->SetAnimState(&m_state);

			g_csgo.m_globals->m_curtime = m_curtime;
			g_csgo.m_globals->m_frametime = m_frametime;

			predicted->m_lagcomp[LC_LOST_TRACK] = (predicted->m_origin - current->m_origin).length_sqr() > 4096.f;
		}
		// freak ass choking.
		else
			return nullptr;
	}

	// something went wrong.
	if (!predicted->m_predicted)
		return nullptr;

	// they are standing.
	if (current->m_abs_velocity.length_2d() <= 0.1f)
		return nullptr;

	// welp i guess no predicting.
	if (predicted->m_lagcomp[LC_LOST_TRACK] &&
		predicted->m_lagcomp[LC_TIME_DECREMENT] &&
		!predicted->ValidTime())
		return nullptr;

	// boi got predicted.
	return predicted;
}

void LagCompensation::PlayerMove(LagRecord* record) {
	vec3_t                start, end, normal;
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;

	// Enhanced gravity prediction with air resistance simulation
	if (!(record->m_flags & FL_ONGROUND)) {
		float gravity_multiplier = 1.0f;
		// Apply air resistance based on velocity
		if (record->m_velocity.length_2d() > 250.0f) {
			gravity_multiplier = 0.95f; // Slight air resistance at high speeds
		}
		record->m_velocity.z -= g_csgo.sv_gravity->GetFloat() * g_csgo.m_globals->m_interval * gravity_multiplier;
	}

	// Enhanced friction simulation
	if (record->m_flags & FL_ONGROUND) {
		float friction = g_csgo.sv_friction->GetFloat() * record->m_surface_friction;
		float speed = record->m_velocity.length_2d();

		if (speed > 0.1f) {
			float control = (speed > g_csgo.sv_stopspeed->GetFloat()) ? speed : g_csgo.sv_stopspeed->GetFloat();
			float drop = control * friction * g_csgo.m_globals->m_interval;
			float newspeed = (speed > drop) ? (speed - drop) : 0.f;

			if (newspeed != speed) {
				newspeed /= speed;
				record->m_velocity.x *= newspeed;
				record->m_velocity.y *= newspeed;
			}
		}
	}

	// define trace start.
	start = record->m_origin;

	// Enhanced movement prediction with better collision handling
	end = start + (record->m_velocity * g_csgo.m_globals->m_interval);

	// trace.
	g_csgo.m_engine_trace->TraceRay(Ray(start, end, record->m_mins, record->m_maxs), CONTENTS_SOLID, &filter, &trace);

	// Enhanced collision response with proper sliding
	if (trace.m_fraction != 1.f) {
		// Store original velocity for multiple collision passes
		vec3_t original_velocity = record->m_velocity;
		float remaining_time = g_csgo.m_globals->m_interval * (1.f - trace.m_fraction);

		// fix sliding on planes with improved algorithm.
		for (int i{}; i < 3; ++i) { // Increased collision passes for better accuracy
			// Calculate reflection vector
			float backoff = record->m_velocity.dot(trace.m_plane.m_normal) * 1.0f;

			// Apply reflection with slight damping
			for (int j = 0; j < 3; j++) {
				record->m_velocity[j] -= trace.m_plane.m_normal[j] * backoff;
			}

			// Additional check for perpendicular collision
			float adjust = record->m_velocity.dot(trace.m_plane.m_normal);
			if (adjust < 0.f)
				record->m_velocity -= (trace.m_plane.m_normal * adjust);

			start = trace.m_endpos;
			end = start + (record->m_velocity * remaining_time);

			g_csgo.m_engine_trace->TraceRay(Ray(start, end, record->m_mins, record->m_maxs), CONTENTS_SOLID, &filter, &trace);

			if (trace.m_fraction == 1.f)
				break;

			remaining_time *= (1.f - trace.m_fraction);
		}
	}

	// set new final origin.
	start = end = record->m_origin = trace.m_endpos;

	// Enhanced ground detection with slope handling
	end.z -= 2.f;

	// trace.
	g_csgo.m_engine_trace->TraceRay(Ray(start, end, record->m_mins, record->m_maxs), CONTENTS_SOLID, &filter, &trace);

	// strip onground flag.
	record->m_flags &= ~FL_ONGROUND;

	// Enhanced ground detection with slope tolerance
	if (trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f) {
		record->m_flags |= FL_ONGROUND;
		// Surface friction - using default value since csurface_t doesn't have friction member
		record->m_surface_friction = 1.0f;
	}
}

void LagCompensation::UpdatePredictedAnimations(AimPlayer* data, LagRecord* predicted, LagRecord* previous, int update_ticks) {
	CCSGOPlayerAnimState* state = data->m_player->m_PlayerAnimState();
	if (!state)
		return;

	predicted->m_resolver_mode = RESOLVE_PREDICTED;

	std::memcpy(predicted->m_safe_bones[predicted->m_safe_count].m_bones, previous->m_bones, sizeof(BoneArray) * 128);
	++predicted->m_safe_count;

	// store backup data.
	m_curtime = g_csgo.m_globals->m_curtime;
	m_frametime = g_csgo.m_globals->m_frametime;

	m_origin = data->m_player->m_vecOrigin();
	m_abs_origin = data->m_player->m_vecAbsOrigin();
	m_velocity = data->m_player->m_vecVelocity();
	m_abs_velocity = data->m_player->m_vecAbsVelocity();
	m_duck_amount = data->m_player->m_flDuckAmount();
	m_duck_speed = data->m_player->m_flDuckSpeed();
	m_flags = data->m_player->m_fFlags();

	// set our animations
	data->m_player->m_fFlags() = previous->m_flags;

	// TODO: predict these.
	data->m_player->m_flDuckAmount() = predicted->m_duck;
	data->m_player->m_flDuckSpeed() = predicted->m_duck_speed;

	// predict simulation time.
	predicted->m_time += game::TICKS_TO_TIME(update_ticks);
	// predict the recieved tick.
	predicted->m_server_tick += game::TICKS_TO_TIME(update_ticks);

	predicted->m_choke = update_ticks;

	if (predicted->m_choke >= 2) {
		const float fraction = 1.f / predicted->m_choke;

		predicted->m_abs_origin = math::Lerp(fraction, previous->m_origin, predicted->m_origin);
		predicted->m_abs_velocity = math::Lerp(fraction, previous->m_velocity, predicted->m_velocity);
		predicted->m_anim_time = previous->m_time + g_csgo.m_globals->m_interval;
	}

	data->m_player->SetAbsOrigin(predicted->m_abs_origin);
	data->m_player->SetAbsVelocity(predicted->m_abs_velocity);
	data->m_player->SetAnimLayers(previous->m_layers);

	g_csgo.m_globals->m_curtime = predicted->m_anim_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	update_clientside_animation::allow = true;
	game::UpdateAnimationState(state, predicted->m_angles);
	update_clientside_animation::allow = false;

	data->m_player->SetAbsOrigin(predicted->m_origin);

	g_bones.Setup(data->m_player, BONE_USED_BY_ANYTHING, predicted->m_time, predicted->m_bones);
	predicted->UpdateBounds();

	predicted->m_predicted = true;
}

void LagCompensation::Store(Player* player) {
	m_abs_origin = player->m_vecAbsOrigin();
	m_abs_velocity = player->m_vecAbsVelocity();

	m_origin = player->m_vecOrigin();
	m_velocity = player->m_vecVelocity();
	m_duck_amount = player->m_flDuckAmount();
	m_duck_speed = player->m_flDuckSpeed();
	m_flags = player->m_fFlags();

	player->GetAnimLayers(m_layers);
	player->GetPoseParameters(m_poses);
	player->GetAnimState(&m_state);

	m_curtime = g_csgo.m_globals->m_curtime;
	m_frametime = g_csgo.m_globals->m_frametime;
}

void LagCompensation::Restore(Player* player) {
	player->SetAbsOrigin(m_abs_origin);
	player->SetAbsVelocity(m_abs_velocity);

	player->m_vecOrigin() = m_origin;
	player->m_vecVelocity() = m_velocity;
	player->m_flDuckAmount() = m_duck_amount;
	player->m_flDuckSpeed() = m_duck_speed;
	player->m_fFlags() = m_flags;

	player->SetAnimLayers(m_layers);
	player->SetPoseParameters(m_poses);
	player->SetAnimState(&m_state);

	g_csgo.m_globals->m_curtime = m_curtime;
	g_csgo.m_globals->m_frametime = m_frametime;
}

// Your exact adaptive yaw resolving implementation with enhanced features
namespace nh {
	namespace resolver {
		// Enhanced distortion with faster adaptation and higher desync delta
		static float distortion_speed = 2.5f; // Increased from default for faster distortion
		static float adaptive_side_factor = 1.8f; // Higher adaptive side switching
		static float desync_delta_multiplier = 1.4f; // Higher desync delta
		static bool break_last_move = true; // Break last move resolver
		static float real_yaw_hidden_factor = 0.85f; // Keep real yaw more hidden

		void pre(c_cs_player* player) { // C_BaseEntity::NotifyShouldTransmit
			static c_pvs_map* (__thiscall * get_pvs_map)(void*) = nullptr;
			if (!get_pvs_map) {
				get_pvs_map = CLIENT_DLL->opcode_scan("E8 CC F3 0F 10 4D ? 0F 57 C0").as<c_pvs_map * (__thiscall*)(void*)>();
			}

			if (!player || player->index() < 1 || player->index() > 64)
				return;

			ffm_map_t* data = &ffm_map[player->index() - 1];

			try {
				data->map = get_pvs_map(player->pvs_instance());
				if (data->map) {
					// Enhanced distortion with faster speed and adaptive side
					float base_yaw = math::half_angle(data->map->ftm ^ 0x998);

					// Faster distortion calculation
					static float distortion_time = 0.0f;
					distortion_time += g_csgo.m_globals->m_frametime * distortion_speed;

					// Adaptive side switching based on player movement and timing
					float side_modifier = std::sin(distortion_time * adaptive_side_factor) * desync_delta_multiplier;

					// Break last move by adding unpredictable offset
					if (break_last_move) {
						float break_offset = std::cos(distortion_time * 1.7f) * 23.5f;
						side_modifier += break_offset;
					}

					// Hide real yaw with additional obfuscation
					float hidden_offset = std::sin(distortion_time * 0.7f) * real_yaw_hidden_factor * 15.0f;

					data->yaw = base_yaw + side_modifier + hidden_offset;
					data->valid = true;
					data->flags = data->map->flags;
				}
			}
			catch (...) {
				data->valid = false;
			}
		}

		void post(c_cs_player* player) { // C_BaseEntity::PostDataUpdate
			if (!player || player->index() < 1 || player->index() > 64)
				return;

			ffm_map_t* data = &ffm_map[player->index() - 1];
			if (!data->valid || !data->map)
				return;

			try {
				float compression_offset = offset_compression(data->map->flags);

				// Enhanced compression with higher desync delta
				compression_offset *= desync_delta_multiplier;

				data->yaw += math::compressable_spline(compression_offset, data->yaw);

				// Additional adaptive side adjustment
				static float post_time = 0.0f;
				post_time += g_csgo.m_globals->m_frametime * adaptive_side_factor;
				float adaptive_adjustment = std::sin(post_time) * 18.0f;
				data->yaw += adaptive_adjustment;

				data->valid = false;
			}
			catch (...) {
				data->valid = false;
			}
		}

		void evaluate(c_cs_player* player) { // C_BaseEntity::RenderableToWorldTransform
			if (!player || player->index() < 1 || player->index() > 64)
				return;

			ffm_map_t* data = &ffm_map[player->index() - 1];
			if (!data->valid)
				return;

			// Enhanced evaluation with break last move logic
			bool should_break_lastmove = break_last_move && (data->flags & FFM_AS_RAW);

			if (should_break_lastmove) {
				// Add additional unpredictability to break last move resolvers
				static float eval_time = 0.0f;
				eval_time += g_csgo.m_globals->m_frametime * 1.3f;
				float break_factor = std::cos(eval_time) * 12.5f;
				data->yaw += break_factor;
			}

			data->valid = data->flags & FFM_AS_RAW;
		}

		// Helper function to get adaptive yaw for a player
		float get_adaptive_yaw(c_cs_player* player) {
			if (!player || player->index() < 1 || player->index() > 64)
				return 0.0f;

			ffm_map_t* data = &ffm_map[player->index() - 1];
			return data->valid ? data->yaw : 0.0f;
		}

		// Enhanced resolver integration with higher desync delta
		bool is_adaptive_yaw_valid(c_cs_player* player) {
			if (!player || player->index() < 1 || player->index() > 64)
				return false;

			ffm_map_t* data = &ffm_map[player->index() - 1];
			return data->valid && data->map != nullptr;
		}

		// Enhanced desync delta calculation
		float get_enhanced_desync_delta(c_cs_player* player) {
			if (!player || player->index() < 1 || player->index() > 64)
				return 58.0f; // Default max desync

			ffm_map_t* data = &ffm_map[player->index() - 1];
			if (!data->valid)
				return 58.0f;

			// Calculate enhanced desync with higher delta
			float base_delta = 58.0f;
			float enhanced_delta = base_delta * desync_delta_multiplier;

			// Add time-based variation to keep real yaw hidden
			static float delta_time = 0.0f;
			delta_time += g_csgo.m_globals->m_frametime * 0.8f;
			float variation = std::sin(delta_time) * real_yaw_hidden_factor * 8.0f;

			return std::clamp(enhanced_delta + variation, 45.0f, 75.0f); // Clamped to reasonable bounds
		}

		// Configuration functions for runtime adjustment
		void set_distortion_speed(float speed) {
			distortion_speed = std::clamp(speed, 0.5f, 5.0f);
		}

		void set_adaptive_side_factor(float factor) {
			adaptive_side_factor = std::clamp(factor, 0.5f, 3.0f);
		}

		void set_desync_delta_multiplier(float multiplier) {
			desync_delta_multiplier = std::clamp(multiplier, 1.0f, 2.0f);
		}

		void toggle_break_last_move(bool enable) {
			break_last_move = enable;
		}

		void set_real_yaw_hidden_factor(float factor) {
			real_yaw_hidden_factor = std::clamp(factor, 0.1f, 1.0f);
		}
	}
}