#include "includes.h"
#include <random>

Aimbot g_aimbot{ };
WeaponSettings m_settings;

void Element::AddShowWpnCallback(int i) {
	switch (i) {
	case 0:
		m_show_callbacks.push_back(callbacks::IsWeaponAuto);
		break;
	case 1:
		m_show_callbacks.push_back(callbacks::IsWeaponScout);
		break;
	case 2:
		m_show_callbacks.push_back(callbacks::IsWeaponAwp);
		break;
	case 3:
		m_show_callbacks.push_back(callbacks::IsWeaponHPistol);
		break;
	case 4:
		m_show_callbacks.push_back(callbacks::IsWeaponPistol);
		break;
	default:
		m_show_callbacks.push_back(callbacks::IsWeaponDefault);
		break;
	}
}

//int ResolvePlayerVelocity(C_AnimationLayer* animlayers, LagRecord* previous, Player* player)
//{
//	if (!animlayers)
//		return 0;
//
//	const auto& jump_or_fall_layer = animlayers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL];
//	const auto& move_layer = animlayers[ANIMATION_LAYER_MOVEMENT_MOVE];
//	const auto& land_or_climb_layer = animlayers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB];
//	const auto& alive_loop_layer = animlayers[ANIMATION_LAYER_ALIVELOOP];
//
//	const bool on_ground = (player->m_fFlags() & FL_ONGROUND) != 0;
//	vec3_t& velocity = player->m_vecVelocity();
//
//	if (on_ground && move_layer.weight == 0.0f) {
//		velocity.zero();
//		return 1;
//	}
//
//	const float dt = std::max(g_csgo.m_globals->m_interval, player->m_flSimulationTime() - previous->m_sim_time);
//	vec3_t avg_velocity = (player->m_vecOrigin() - previous->m_origin) / dt;
//
//	if (previous->m_velocity.length_2d() <= 0.1f || dt <= g_csgo.m_globals->m_interval) {
//		velocity = avg_velocity;
//		return 2;
//	}
//
//	const auto* weapon_info = player->GetActiveWeapon() ? player->GetActiveWeapon()->GetWpnData() : nullptr;
//	float max_weapon_speed = weapon_info
//		? (player->m_bIsScoped() ? weapon_info->m_max_player_speed : weapon_info->m_max_player_speed_alt)
//		: 250.0f;
//
//	if (on_ground) {
//		avg_velocity.z = 0.0f;
//		velocity = avg_velocity;
//
//		int detail = 0;
//
//		if (move_layer.playback_rate > 0.0f) {
//			vec3_t direction = velocity.normalized();
//			direction.z = 0.0f;
//
//			float avg_speed_xy = velocity.length_2d();
//			float move_weight = move_layer.weight;
//			float blended_modifier = math::Lerp(CS_PLAYER_SPEED_WALK_MODIFIER, CS_PLAYER_SPEED_DUCK_MODIFIER, player->m_flDuckAmount());
//			float target_speed = max_weapon_speed * blended_modifier * move_weight;
//			float run_scale = 0.35f * (1.0f - alive_loop_layer.weight);
//
//			if (alive_loop_layer.weight > 0.0f && alive_loop_layer.weight < 1.0f) {
//				velocity = direction * (max_weapon_speed * (run_scale + 0.55f));
//				return 3;
//			}
//			else if (move_weight < 0.95f || target_speed > avg_speed_xy) {
//				velocity = direction * target_speed;
//
//				float prev_weight = previous->m_layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight;
//				float weight_rate = (move_layer.weight - prev_weight) / dt;
//				bool walking = velocity.length_2d() > max_weapon_speed * CS_PLAYER_SPEED_WALK_MODIFIER;
//
//				if (move_layer.weight == prev_weight)
//					detail = 4;
//				else if (move_layer.weight > prev_weight)
//					detail = 5;
//			}
//			else {
//				float speed = max_weapon_speed * move_weight;
//
//				if ((player->m_fFlags() & FL_DUCKING) != 0)
//					speed *= CS_PLAYER_SPEED_DUCK_MODIFIER;
//				else if (player->m_bIsWalking())
//					speed *= CS_PLAYER_SPEED_WALK_MODIFIER;
//
//				if (avg_speed_xy > speed) {
//					velocity = direction * speed;
//
//					float prev_weight = previous->m_layers[ANIMATION_LAYER_MOVEMENT_MOVE].weight;
//					float weight_rate = (move_layer.weight - prev_weight) / dt;
//
//					if (move_layer.weight == prev_weight)
//						detail = 4;
//					else if (move_layer.weight > prev_weight)
//						detail = 5;
//				}
//			}
//		}
//
//		return detail;
//	}
//	else {
//		const bool crouch = player->m_flDuckAmount() > 0.0f;
//		const float walk_speed_ratio = avg_velocity.length_2d() / (max_weapon_speed * CS_PLAYER_SPEED_WALK_MODIFIER);
//		const bool moving = walk_speed_ratio > 0.25f;
//
//		int seq = moving + (crouch ? 17 : 15);
//
//		velocity = avg_velocity;
//
//		if (jump_or_fall_layer.weight > 0.0f &&
//			jump_or_fall_layer.playback_rate > 0.0f &&
//			jump_or_fall_layer.sequence == seq) {
//			float jump_time = jump_or_fall_layer.cycle * jump_or_fall_layer.playback_rate;
//			velocity.z = g_csgo.sv_jump_impulse->GetFloat() - jump_time * g_csgo.sv_gravity->GetFloat() * 0.5f;
//		}
//
//		return 2;
//	}
//
//	player->m_vecVelocity() = velocity;
//	return 0;
//}

// Accurate hitchance calculation with proper spread simulation
float Aimbot::GetHitchance(AimPlayer* data, LagRecord* record, const vec3_t& aim_point, float distance) {
	if (!g_cl.m_weapon || !g_cl.m_weapon_info || !data || !record)
		return 0.f;

	// Get weapon accuracy values
	float weapon_inaccuracy = g_cl.m_weapon_info->m_inaccuracy_stand;
	float weapon_spread = g_cl.m_weapon_info->m_spread;

	// Apply movement inaccuracy
	vec3_t local_velocity = g_cl.m_local->m_vecVelocity();
	float velocity_2d = local_velocity.length_2d();
	
	if (velocity_2d > 5.f) {
		// Use crouch/walk inaccuracy based on movement state
		if (g_cl.m_local->m_fFlags() & FL_DUCKING) {
			weapon_inaccuracy = g_cl.m_weapon_info->m_inaccuracy_crouch;
		} else {
			// Linear interpolation between stand and move inaccuracy
			float move_factor = std::min(velocity_2d / 250.f, 1.f);
			weapon_inaccuracy = math::Lerp(weapon_inaccuracy, g_cl.m_weapon_info->m_inaccuracy_move, move_factor);
		}
	}

	// Apply recoil inaccuracy
	if (g_cl.m_local && g_cl.m_local->m_aimPunchAngle().length() > 0.1f) {
		float recoil_index = g_cl.m_weapon->m_flRecoilIndex();
		weapon_inaccuracy += recoil_index * 0.01f; // Use constant instead of missing member
	}

	// Calculate total inaccuracy (combine inaccuracy and spread)
	float total_inaccuracy = weapon_inaccuracy + weapon_spread;

	// Convert to angle in radians
	float spread_angle = std::atan(total_inaccuracy);
	
	// Calculate spread radius at target distance
	float spread_radius = distance * std::tan(spread_angle);
	
	// Get hitbox radius (simplified - you might want to use actual hitbox dimensions)
	float hitbox_radius = 4.f; // Default for head
	
	// Estimate hitbox based on aim point height relative to player origin
	float height_diff = aim_point.z - record->m_origin.z;
	if (height_diff > 50.f) {
		hitbox_radius = 4.f; // Head
	} else if (height_diff > 20.f) {
		hitbox_radius = 8.f; // Upper chest/chest
	} else if (height_diff > -10.f) {
		hitbox_radius = 6.f; // Body/pelvis
	} else {
		hitbox_radius = 5.f; // Legs
	}

	// Calculate hitchance using circular probability
	if (spread_radius <= 0.1f) {
		return 1.f; // Perfect accuracy
	}
	
	float radius_ratio = hitbox_radius / spread_radius;
	
	if (radius_ratio >= 1.f) {
		return 1.f; // Guaranteed hit
	}
	
	// Use proper circular probability distribution
	// This represents the probability that a random point in the spread circle
	// falls within the hitbox circle
	float hitchance = radius_ratio * radius_ratio;
	
	// Apply additional factors
	
	// Resolver confidence penalty
	switch (record->m_resolver_mode) {
	case RESOLVE_NONE:
	case RESOLVE_UPDATE:
		// High confidence - no penalty
		break;
	case RESOLVE_MOVE:
		hitchance *= 0.95f;
		break;
	case RESOLVE_STAND:
		hitchance *= 0.9f;
		break;
	case RESOLVE_AIR:
		hitchance *= 0.85f;
		break;
	case RESOLVE_PREDICTED:
	case RESOLVE_LASTMOVE:
	case RESOLVE_FAKE:
		hitchance *= 0.8f;
		break;
	default:
		hitchance *= 0.75f;
		break;
	}
	
	// Lag compensation penalty
	if (record->m_choke > 8) {
		hitchance *= 0.9f;
	} else if (record->m_choke > 14) {
		hitchance *= 0.8f;
	}
	
	// Target movement penalty
	float target_speed = record->m_velocity.length_2d();
	if (target_speed > 100.f) {
		float movement_penalty = 1.f - (target_speed / 1000.f) * 0.3f;
		hitchance *= std::max(movement_penalty, 0.7f);
	}
	
	return std::clamp(hitchance, 0.f, 1.f);
}

// Add this method to the aimbot class
float Aimbot::CalculateHitchance(AimPlayer* data, LagRecord* record) {
	if (!data || !record)
		return 0.f;

	// Get distance to target
	float distance = (record->m_origin - g_cl.m_shoot_pos).length();
	
	// Use enhanced hitchance calculation
	vec3_t dummy_pos = record->m_origin; // Use record origin as aim point
	return GetHitchance(data, record, dummy_pos, distance);
}

void AimPlayer::SyncPlayerVelocity(LagRecord* current, LagRecord* previous) {
	const C_AnimationLayer* move = &current->m_layers[ANIMATION_LAYER_MOVEMENT_MOVE];
	const C_AnimationLayer* prev_move = previous ? &previous->m_layers[ANIMATION_LAYER_MOVEMENT_MOVE] : nullptr;
	const C_AnimationLayer* alive_loop = &current->m_layers[ANIMATION_LAYER_ALIVELOOP];
	const C_AnimationLayer* prev_alive_loop = previous ? &previous->m_layers[ANIMATION_LAYER_ALIVELOOP] : nullptr;

	const Weapon* weapon = m_player->GetActiveWeapon();

	if (previous && !(current->m_anim_flags & FL_ONGROUND) && (previous->m_anim_flags & FL_ONGROUND))
		current->m_abs_velocity.z = g_csgo.sv_jump_impulse->GetFloat() - (m_player->m_flGravity() * g_csgo.sv_gravity->GetFloat() * game::TICKS_TO_TIME(current->m_choke) * 0.5f);

	// we need to store this since it will get overriden.
	float z = current->m_abs_velocity.z;

	if (move->m_playback_rate <= 0.0f) {
		current->m_abs_velocity = { 0.f, 0.f, z };
		std::memset(&m_state->m_velocity, 0.f, sizeof(float) * 2);
		return;
	}

	// check if we can read velocity from alive loop.
	if (prev_alive_loop &&
		weapon && weapon == m_state->m_weapon &&
		alive_loop->m_weight > 0.55f && alive_loop->m_weight < 0.9f &&
		alive_loop->m_playback_rate == prev_alive_loop->m_playback_rate &&
		alive_loop->m_sequence == prev_alive_loop->m_sequence) {
		float speed_as_portion_of_run_top_speed = (1.f - alive_loop->m_weight) * (0.9f - 0.55f);
		if (speed_as_portion_of_run_top_speed > 0.f) {
			speed_as_portion_of_run_top_speed += 0.55f;

			current->m_abs_velocity = current->m_abs_velocity.normalized() * speed_as_portion_of_run_top_speed * m_max_speed;
			current->m_abs_velocity.z = z;

			std::memcpy(&m_state->m_velocity, &current->m_abs_velocity, sizeof(float) * 2);
			return;
		}
	}
}

void AimPlayer::HandleSafePoints(LagRecord* current, LagRecord* previous) {
	static vec3_t               abs_origin;
	static CCSGOPlayerAnimState state;
	static C_AnimationLayer     layers[13];

	abs_origin = m_player->m_vecAbsOrigin();
	m_player->GetAnimState(&state);
	m_player->GetAnimLayers(layers);

	if (current->IsResolved()) {
		// setup safepoint angles.
		if (!previous || !previous->IsResolved()) {
			m_foot_data[0].m_foot_yaw = current->m_angles.y;
			m_foot_data[1].m_foot_yaw = current->m_angles.y + 58.f;
			m_foot_data[2].m_foot_yaw = current->m_angles.y - 58.f;
		}

		for (int i = 0; i < 3; i++) {
			m_foot_data[i].Set(m_state);

			update_clientside_animation::allow = true;
			game::UpdateAnimationState(m_state, current->m_angles);
			update_clientside_animation::allow = false;

			m_foot_data[i].Store(m_state);

			m_player->SetAbsOrigin(current->m_origin);
			m_player->SetAnimLayers(current->m_layers);

			// CRASH FIX: Safe bone setup for safe points
			try {
				g_bones.Setup(m_player, BONE_USED_BY_ANYTHING, current->m_time, current->m_safe_bones[i].m_bones);
			}
			catch (...) {
				// Safe bone setup failed, skip this iteration
				continue;
			}
			++current->m_safe_count;

			m_player->SetAnimState(&state);
			m_player->SetAnimLayers(layers);
			m_player->SetAbsOrigin(abs_origin);
		}
		return;
	}

	ang_t away;
	math::VectorAngles(current->m_origin - g_cl.m_local->m_vecAbsOrigin(), away);
	for (int i = 0; i < 8; i++) {
		update_clientside_animation::allow = true;
		game::UpdateAnimationState(m_state, { current->m_angles.x, math::NormalizedAngle(away.y + 45.f * i), 0.f });
		update_clientside_animation::allow = false;

		m_player->SetAbsOrigin(current->m_origin);
		m_player->SetAnimLayers(current->m_layers);

		// CRASH FIX: Safe bone setup for away angles
		try {
			g_bones.Setup(m_player, BONE_USED_BY_ANYTHING, current->m_time, current->m_safe_bones[i].m_bones);
			++current->m_safe_count;
		}
		catch (...) {
			// Safe bone setup failed, skip this iteration
			continue;
		}

		m_player->SetAnimState(&state);
		m_player->SetAnimLayers(layers);
		m_player->SetAbsOrigin(abs_origin);
	}
}

void AimPlayer::UpdateAnimations(LagRecord* current, LagRecord* previous) {
	// CRASH FIX: Validate pointers
	if (!current || !m_player) {
		return;
	}

	current->Store(m_player);

	m_state = m_player->m_PlayerAnimState();
	if (!m_state)
		return;

	m_state->m_player = m_player;

	const float curtime = g_csgo.m_globals->m_curtime;
	const float frametime = g_csgo.m_globals->m_frametime;

	const float lower_body_yaw = m_player->m_flLowerBodyYawTarget();
	const float duck_amount = m_player->m_flDuckAmount();
	const float duck_speed = m_player->m_flDuckSpeed();

	const int flags = m_player->m_fFlags();
	const int eflags = m_player->m_iEFlags();
	const int move_state = m_player->m_iMoveState();
	const int move_type = m_player->m_MoveType();

	const vec3_t abs_origin = m_player->m_vecAbsOrigin();
	const vec3_t origin = m_player->m_vecOrigin();
	const vec3_t abs_velocity = m_player->m_vecAbsVelocity();
	const vec3_t velocity = m_player->m_vecVelocity();

	const EHANDLE weapon_handle = m_player->m_hActiveWeapon();

	m_max_speed = 260.f;

	// player respawned.
	if (m_player->m_flSpawnTime() != m_spawn) {
		// reset animation state.
		game::ResetAnimationState(m_state);

		// note new spawn time.
		m_spawn = m_player->m_flSpawnTime();
	}

	m_state->m_strafe_sequence = current->m_layers[7].m_sequence;

	Weapon* weapon = nullptr;

	if (previous) {
		m_state->m_last_update_time = previous->m_anim_time;
		m_state->m_primary_cycle = previous->m_layers[6].m_cycle;
		m_state->m_move_weight = previous->m_layers[6].m_weight;
		m_state->m_strafe_change_weight = previous->m_layers[7].m_weight;
		m_state->m_strafe_change_cycle = previous->m_layers[7].m_cycle;

		m_player->SetAnimLayers(previous->m_layers);
		m_player->m_flLowerBodyYawTarget() = previous->m_body;

		current->m_choke = game::TIME_TO_TICKS(current->m_time - previous->m_time);

		if (current->m_choke >= 1) {
			m_player->m_hActiveWeapon() = previous->m_weapon_handle;

			current->m_anim_time = previous->m_time + g_csgo.m_globals->m_interval;
			current->m_velocity = (current->m_origin - previous->m_origin) / game::TICKS_TO_TIME(current->m_choke);

			// the player is choking.
			if (current->m_choke >= 2) {
				const float fraction = 1.f / current->m_choke;

				current->m_abs_origin = math::Lerp(fraction, previous->m_origin, current->m_origin);
				current->m_abs_velocity = math::Lerp(fraction, previous->m_velocity, current->m_velocity);

				m_player->m_flDuckAmount() = math::Lerp(fraction, previous->m_duck, current->m_duck);
				m_player->m_flDuckSpeed() = math::Lerp(fraction, previous->m_duck_speed, current->m_duck_speed);

				m_player->m_iMoveState() = previous->m_move_state;
				m_player->m_MoveType() = previous->m_move_type;

				current->m_anim_flags = previous->m_flags;
			}
		}

		weapon = m_player->GetActiveWeapon();

		if (m_player->m_MoveType() != MOVETYPE_LADDER) {
			if (current->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_cycle > previous->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_cycle &&
				current->m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_cycle == previous->m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_cycle) {
				current->m_anim_flags &= ~FL_ONGROUND;
			}
			else {
				if (m_player->GetLayerActivity(&current->m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB]) != ACT_CSGO_CLIMB_LADDER &&
					current->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_cycle > previous->m_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_cycle &&
					current->m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_cycle > previous->m_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_cycle) {
					current->m_anim_flags |= FL_ONGROUND;
				}
			}
		}

		if (current->m_body != previous->m_body)
			previous->m_anim_flags |= FL_ONGROUND;

		if (weapon) {
			WeaponInfo* weapon_info = weapon->GetWpnData();
			if (weapon_info)
				m_max_speed = m_player->m_bIsScoped() ? weapon_info->m_max_player_speed_alt : weapon_info->m_max_player_speed;
		}

		SyncPlayerVelocity(current, previous);

		if ((current->m_origin - previous->m_origin).length_sqr() > 4096.f)
			current->m_lagcomp[LC_LOST_TRACK] = true;

		if (current->m_time < previous->m_time)
			m_last_shift_time = previous->m_time;
	}
	// pvs fix.
	else {
		weapon = m_player->GetActiveWeapon();

		for (int i = 0; i < 3; i++)
			m_foot_data[i].Store(m_state);

		m_state->m_last_update_time = current->m_time - g_csgo.m_globals->m_interval;
		m_state->m_abs_yaw = current->m_body;
		m_state->m_velocity = current->m_velocity;
		m_state->m_duration_in_air = (current->m_anim_flags & FL_ONGROUND) ? 0.f : 1.f;
	}

	if (current->m_time <= m_last_shift_time)
		current->m_lagcomp[LC_TIME_DECREMENT] = true;

	m_anim_velocity = math::Approach(current->m_abs_velocity, m_state->m_velocity, game::TICKS_TO_TIME(current->m_choke) * 2000);

	m_player->m_iEFlags() &= ~(EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY);
	m_player->m_fFlags() = current->m_anim_flags;
	m_player->SetAbsVelocity(current->m_abs_velocity);
	m_player->SetAbsOrigin(current->m_abs_origin);

	g_csgo.m_globals->m_curtime = current->m_anim_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	g_resolver.ResolveAngles(this, current, previous);

	m_state->m_last_update_frame = 0;
	m_state->m_weapon = weapon;

	HandleSafePoints(current, previous);

	update_clientside_animation::allow = true;
	game::UpdateAnimationState(m_state, current->m_angles);

	if (m_player->m_nSequence() != -1)
		on_latch_interpolated_vars::original(m_player, 1);

	update_clientside_animation::allow = false;

	current->m_abs_angles = { 0.f, m_state->m_abs_yaw, 0.f };
	m_player->GetPoseParameters(current->m_poses);

	m_player->SetAnimLayers(current->m_layers);
	m_player->SetAbsOrigin(current->m_origin);

	m_player->m_hActiveWeapon() = weapon_handle;

	m_player->m_flLowerBodyYawTarget() = lower_body_yaw;

	m_player->m_flDuckAmount() = duck_amount;
	m_player->m_flDuckSpeed() = duck_speed;

	m_player->m_fFlags() = flags;
	m_player->m_iEFlags() = eflags;

	m_player->m_iMoveState() = move_state;
	m_player->m_MoveType() = move_type;

	// CRASH FIX: Safe bone setup
	try {
		g_bones.Setup(m_player, BONE_USED_BY_ANYTHING, current->m_time, current->m_bones);
		current->UpdateBounds();
	}
	catch (...) {
		// Bone setup failed, use default pose
		std::memset(current->m_bones, 0, sizeof(current->m_bones));
	}

	m_player->SetAbsOrigin(abs_origin);
	m_player->m_vecOrigin() = origin;
	m_player->SetAbsVelocity(abs_velocity);
	m_player->m_vecVelocity() = velocity;

	g_csgo.m_globals->m_curtime = curtime;
	g_csgo.m_globals->m_frametime = frametime;
}

void AimPlayer::OnNetUpdate(Player* player) {
	// CRASH FIX: Additional validation
	if (!player || player->m_bIsLocalPlayer()) {
		Reset();
		return;
	}

	if (!player->alive() || player->dormant()) {
		Reset();
		return;
	}

	// CRASH FIX: Validate player index
	if (player->index() < 1 || player->index() > 64) {
		Reset();
		return;
	}

	if (player != m_player) {
		Reset();
	}

	m_player = player;

	// CRASH FIX: Safe container access
	LagRecord* previous = nullptr;
	try {
		if (!m_records.empty()) {
			previous = &m_records.front();

			// CRASH FIX: Validate previous record
			if (!previous) {
				return;
			}

			if (player->m_flSimulationTime() == previous->m_time)
				return;

			// CRASH FIX: Bounds check animation overlay
			if (player->m_AnimOverlay() && previous->m_layers &&
				player->m_AnimOverlay()[11].m_cycle == previous->m_layers[11].m_cycle)
				return;
		}
	} catch (...) {
		// Container access failed, reset
		Reset();
		return;
	}

	// CRASH FIX: Safe record creation with container size limit
	try {
		// Limit container size before adding new elements
		const size_t max_records = static_cast<size_t>(1.f / g_csgo.m_globals->m_interval);
		while (m_records.size() >= max_records) {
			m_records.pop_back();
		}
		
		LagRecord& new_record = m_records.emplace_front();
		UpdateAnimations(&new_record, previous);
	} catch (const std::exception& e) {
		// Failed to create/update record, clean up
		if (!m_records.empty()) {
			m_records.pop_front();
		}
		return;
	} catch (...) {
		// Failed to create/update record, clean up  
		if (!m_records.empty()) {
			m_records.pop_front();
		}
		return;
	}
}

bool Aimbot::IsValidTarget(Player* player) {
	if (!player || !player->alive() || player->dormant())
		return false;

	return g_cl.m_local && player->enemy(g_cl.m_local);
}

void Aimbot::SetupSettings() {
	int cfg = 5;

	switch (g_cl.m_weapon_id) {
	case G3SG1:
	case SCAR20:
		cfg = 0;
		break;
	case SSG08:
		cfg = 1;
		break;
	case AWP:
		cfg = 2;
		break;
	case DEAGLE:
	case REVOLVER:
		cfg = 3;
		break;
	case GLOCK:
	case P2000:
	case USPS:
	case FIVESEVEN:
	case TEC9:
	case ELITE:
	case P250:
	case CZ75A:
		cfg = 4;
		break;
	}

	m_settings = g_menu.main.aimbot.settings[cfg];
}

void Aimbot::SetupHitboxes() {
	m_hitboxes.clear();

	if (m_settings.hitboxes.get(0))
		m_hitboxes.emplace_back(HITBOX_HEAD);

	if (m_settings.hitboxes.get(1)) {
		m_hitboxes.emplace_back(HITBOX_UPPER_CHEST);
		m_hitboxes.emplace_back(HITBOX_CHEST);
		m_hitboxes.emplace_back(HITBOX_THORAX);
	}

	if (m_settings.hitboxes.get(2)) {
		m_hitboxes.emplace_back(HITBOX_L_FOREARM);
		m_hitboxes.emplace_back(HITBOX_L_UPPER_ARM);

		m_hitboxes.emplace_back(HITBOX_R_FOREARM);
		m_hitboxes.emplace_back(HITBOX_R_UPPER_ARM);
	}

	if (m_settings.hitboxes.get(3)) {
		m_hitboxes.emplace_back(HITBOX_BODY);
		m_hitboxes.emplace_back(HITBOX_PELVIS);
	}

	if (m_settings.hitboxes.get(4)) {
		m_hitboxes.emplace_back(HITBOX_L_CALF);
		m_hitboxes.emplace_back(HITBOX_L_FOOT);

		m_hitboxes.emplace_back(HITBOX_R_CALF);
		m_hitboxes.emplace_back(HITBOX_R_FOOT);
	}
}

void Aimbot::FindTargets() {
	// clear previous targets.
	m_targets.clear();

	// these will be the records we aim at.
	static std::vector<LagRecord*> records;

	for (int i = 1; i <= g_csgo.m_globals->m_max_clients; i++) {
		Player* player = g_csgo.m_entlist->GetClientEntity<Player*>(i);
		if (!IsValidTarget(player))
			continue;

		AimPlayer* data = &m_players[i - 1];
		if (!data || data->m_records.empty())
			continue;

		// this player is whitelisted.
		if (data->m_white_listed)
			continue;

		// clear records from previous target.
		records.clear();

		LagRecord* ideal = nullptr;
		LagRecord* last = nullptr;

		for (LagRecord& record : data->m_records) {
			if (record.m_lagcomp[LC_LOST_TRACK])
				break;

			if (record.m_lagcomp[LC_TIME_DECREMENT])
				continue;

			if (!record.ValidTime())
				continue;

			// first resolved record.
			if (!ideal && record.IsResolved()) {
				ideal = &record;
				continue;
			}

			// last record.
			last = &record;
		}

		if (ideal)
			records.emplace_back(ideal);

		if (last)
			records.emplace_back(last);

		LagRecord* predicted = g_lagcomp.StartPrediction(data);
		if (predicted && (m_settings.force_extrapolation.get() || records.empty()))
			records.emplace_back(predicted);

		if (records.empty())
			continue;

		TargetData* target = &m_targets.emplace_back();
		target->m_player = player;
		target->m_index = i;
		target->m_records = records;
	}


	while (m_targets.size() > g_menu.main.aimbot.max_targets.get()) {
		std::random_device rd;  // a seed source for the random number engine
		std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<> distrib(0, m_targets.size() - 1);

		m_targets.erase(m_targets.begin() + distrib(gen));
	}
}

static vec3_t vertice_list_head[] = {
	vec3_t(0.f, 0.f, 0.f), // center
	vec3_t(0.f, 0.f, 1.3f), // top
	vec3_t(-0.7f, 0.f, 0.90f), // right
	vec3_t(0.7f, 0.f, 0.90f), // left
};

static vec3_t vertice_list_foot[] = {
	vec3_t(0.75f, 0.f, 0.f), // toes
	vec3_t(-0.75f, 0.f, 0.f), // heel
};

void Aimbot::CreatePoints() {
	vec3_t mins, maxs, center,
		to_center, forward, right, up;

	std::vector<vec3_t> sides;

	float aim_height = m_settings.aim_height.get() * 0.01f;
	float head_scale = m_settings.head_scale.get() * 0.01f;
	float body_scale = m_settings.body_scale.get() * 0.01f;

	for (auto it = m_targets.begin(); it != m_targets.end(); ) {
		const model_t* model = it->m_player->GetModel();
		if (!model) {
			it = m_targets.erase(it);
			continue;
		}

		studiohdr_t* studio = g_csgo.m_model_info->GetStudioModel(model);
		if (!studio) {
			it = m_targets.erase(it);
			continue;
		}

		mstudiohitboxset_t* set = studio->GetHitboxSet(it->m_player->m_nHitboxSet());
		if (!set) {
			it = m_targets.erase(it);
			continue;
		}

		for (const int& hitbox : m_hitboxes) {
			mstudiobbox_t* bbox = set->GetHitbox(hitbox);
			if (!bbox)
				continue;

			// create points for each record.
			for (LagRecord* record : it->m_records) {
				math::VectorTransform(bbox->m_maxs, record->m_bones[bbox->m_bone], mins);
				math::VectorTransform(bbox->m_mins, record->m_bones[bbox->m_bone], maxs);

				center = (mins + maxs) * 0.5f;

				// capsule multipoints.
				if (bbox->m_radius > 0.0f) {
					to_center = center - g_cl.m_shoot_pos;
					forward = to_center.normalized();
					right = forward.cross({ 0.0f, 0.0f, 1.0f }).normalized();
					up = right.cross(forward).normalized();

					right *= bbox->m_radius;
					up *= bbox->m_radius;

					switch (hitbox) {
					case HITBOX_HEAD:
						for (const vec3_t& vertices : vertice_list_head)
							it->m_points.emplace_back(center + ((right * vertices.x * head_scale) + (up * vertices.z * aim_height)), record, bbox);
						break;
					case HITBOX_PELVIS:
					case HITBOX_BODY:
						it->m_points.emplace_back(center, record, bbox);
						it->m_points.emplace_back(center + right * body_scale, record, bbox);
						it->m_points.emplace_back(center - right * body_scale, record, bbox);
						break;
					case HITBOX_UPPER_CHEST:
						it->m_points.emplace_back(center + up * body_scale, record, bbox);
						it->m_points.emplace_back(center + right * body_scale * 0.75f + up * body_scale * 0.75f, record, bbox);
						it->m_points.emplace_back(center - right * body_scale * 0.75f + up * body_scale * 0.75f, record, bbox);
						break;
					case HITBOX_THORAX:
					case HITBOX_CHEST:
						it->m_points.emplace_back(center + right * body_scale, record, bbox);
						it->m_points.emplace_back(center - right * body_scale, record, bbox);
						break;
					default:
						sides.clear();
						sides.emplace_back(center - right * body_scale * 0.5f);
						sides.emplace_back(center + right * body_scale * 0.5f);

						std::sort(sides.begin(), sides.end(),
							[&](const vec3_t& a, const vec3_t& b) {
								const float& dist_a = (a - record->m_origin).length_2d();
								const float& dist_b = (b - record->m_origin).length_2d();

								return dist_a > dist_b;
							});

						it->m_points.emplace_back(sides.front(), record, bbox);
						break;
					}

					continue;
				}

				// box multipoints.
				it->m_points.emplace_back(center, record, bbox);
			}
		}

		if (it->m_points.empty()) {
			it = m_targets.erase(it);
			continue;
		}

		++it;
	}
}

bool Aimbot::ScanAimPoints(TargetData* target, bool refine) {
	// CRASH FIX: Validate target pointer
	if (!target || !target->m_player) {
		return false;
	}

	m_input.m_start = g_cl.m_shoot_pos;
	m_input.m_from = g_cl.m_local;
	m_input.m_target = target->m_player;
	m_input.m_pen = true;

	int clamped_damage = std::min(m_minimum_damage, target->m_player->m_iHealth());
	ang_t angle;

	for (auto it = target->m_points.begin(); it != target->m_points.end(); ) {
		// CRASH FIX: Validate point record
		if (!it->m_record || !it->m_record->m_player) {
			it = target->m_points.erase(it);
			continue;
		}

		m_input.m_end = it->m_point;

		// CRASH FIX: Safe SetData call
		try {
			it->m_record->SetData();
		}
		catch (...) {
			it = target->m_points.erase(it);
			continue;
		}

		m_output = auto_wall::RunPenetration(&m_input);
		if (m_output.m_damage < clamped_damage) {
			it = target->m_points.erase(it);
			continue;
		}

		if (refine) {
			// if we lost damage ignore the point.
			if (m_output.m_damage < it->m_damage) {
				it = target->m_points.erase(it);
				continue;
			}

			it->m_safety = static_cast<float>(CheckIntersection(&*it)) / it->m_record->m_safe_count;

			// not enough safety.
			if (it->m_safety < static_cast<float>(GetMinimumIntersections(it->m_record)) * 0.01f) {
				it = target->m_points.erase(it);
				continue;
			}

			math::VectorAngles(it->m_point - g_cl.m_shoot_pos, angle);

			// set all the data we need.
			it->m_hitbox = m_output.m_hitbox;
			it->m_hitgroup = m_output.m_hitgroup;
			it->m_hitchance = PointHitchance(&*it, angle, 64);

			// get best hitchance for point sorting.
			if (it->m_hitchance > target->m_best_hitchance)
				target->m_best_hitchance = it->m_hitchance;

			++it;
			continue;
		}

		// get pre autostop damage.
		it->m_damage = m_output.m_damage;

		// get best damage for target sorting.
		if (it->m_damage > target->m_best_damage)
			target->m_best_damage = it->m_damage;

		++it;
	}

	return target->m_points.size() >= 1;
}

PointData* Aimbot::FindBestPoint(TargetData* target) {
	// grab max damage to compare.
	for (int i = HITGROUP_HEAD; i < HITGROUP_GEAR; i++) {
		m_hitgroup_damage.at(i) = g_cl.m_weapon_info->m_damage;

		// scale damage to our targetted hitgroup.
		auto_wall::ScaleDamage(target->m_player, g_cl.m_weapon_info->m_armor_ratio, m_hitgroup_damage.at(i), i);
	}

	// sort them by damage
	std::sort(target->m_points.begin(), target->m_points.end(),
		[&](const PointData& a, const PointData& b) {
			return a.m_damage > b.m_damage;
		});

	float hc_over_dmg = m_settings.hitchance_over_damage.get() * 0.01f;
	float overlap_over_dmg = m_settings.overlap_over_damage.get() * 0.01f;

	// sort by preference.
	std::sort(target->m_points.begin(), target->m_points.end(),
		[&](const PointData& a, const PointData& b) {
			return a.m_hitchance > b.m_hitchance && ((a.m_hitchance / target->m_best_hitchance) + hc_over_dmg) > (b.m_damage / m_hitgroup_damage.at(a.m_hitgroup));
		});

	std::sort(target->m_points.begin(), target->m_points.end(),
		[&](const PointData& a, const PointData& b) {
			return a.m_safety > b.m_safety && (a.m_safety + overlap_over_dmg) > (b.m_damage / m_hitgroup_damage.at(a.m_hitgroup));
		});

	int health = target->m_player->m_iHealth();

	// here we find points that might be crucial.
	for (PointData& point : target->m_points) {
		// dont prefer points that are not 100% safe.
		if (point.m_safety < 1.f)
			continue;

		// ignore points that are not body.
		if (point.m_hitbox < HITBOX_PELVIS || point.m_hitbox > HITBOX_UPPER_CHEST)
			continue;

		// always.
		if (m_settings.prefer_body.get(0))
			return &point;

		// lethal.
		if (m_settings.prefer_body.get(1) && point.m_damage >= health)
			return &point;

		// fake.
		if (m_settings.prefer_body.get(2) && !target->m_points[0].m_record->IsResolved())
			return &point;

		// in air.
		if (m_settings.prefer_body.get(2) && target->m_points[0].m_record->m_resolver_mode == RESOLVE_AIR)
			return &point;
	}

	return &target->m_points[0];
}

void Aimbot::Think() {
	if (!g_menu.main.aimbot.enable.get())
		return;

	if (!g_cl.m_weapon || !g_cl.m_weapon_info)
		return;

	if (g_cl.m_weapon_type == WEAPONTYPE_KNIFE || g_cl.m_weapon_type == WEAPONTYPE_EQUIPMENT)
		return;

	// CRASH FIX: Ensure local player is valid
	if (!g_cl.m_local || !g_cl.m_local->alive()) {
		return;
	}

	ComputeHitchanceSeeds();

	SetupSettings();
	SetupHitboxes();

	m_minimum_damage = std::max(m_damage_override ? g_menu.main.aimbot.damage_override_amt.get() : m_settings.damage.get(), 1.f);

	FindTargets();
	CreatePoints();

	// erase the targets that we cant hit.
	for (auto it = m_targets.begin(); it != m_targets.end(); ) {
		// CRASH FIX: Validate target before processing
		if (!it->m_player || !it->m_player->alive() || it->m_player->dormant()) {
			it = m_targets.erase(it);
			continue;
		}

		if (!ScanAimPoints(&*it)) {
			it = m_targets.erase(it);
			continue;
		}

		++it;
	}

	// we have no targets.
	if (m_targets.empty())
		return;

	m_hitchance = m_settings.hitchance.get() * 0.01f;

	// sort targets.
	std::sort(m_targets.begin(), m_targets.end(),
		[&](const TargetData& a, const TargetData& b) {
			return a.m_best_damage > b.m_best_damage;
		});

	// CRASH FIX: Double check we still have targets after sorting
	if (m_targets.empty()) {
		return;
	}

	TargetData* target = &m_targets[0];

	// CRASH FIX: Additional validation
	if (!target || !target->m_player || !target->m_player->alive()) {
		return;
	}

	// run it one tick early for faster stop.
	if ((g_cl.m_weapon_fire || m_settings.stop_between_shots.get()) &&
		(g_cl.m_local->m_fFlags() & FL_ONGROUND) && !(g_cl.m_buttons & IN_JUMP)) {
		g_movement.AutoStop();

		ang_t dir;
		math::VectorAngles(target->m_points[0].m_point - g_cl.m_shoot_pos, dir);

		g_cl.SetupShootPos(dir.x);
	}

	if (!ScanAimPoints(target, true)) {
		// restore our movement and make sure our pred is correct.
		g_cl.m_cmd->m_forward_move = g_input_pred.m_forward_move;
		g_cl.m_cmd->m_side_move = g_input_pred.m_side_move;
		g_input_pred.repredict();
		return;
	}

	PointData* final_point = FindBestPoint(target);
	if (!final_point)
		return;

	// CRASH FIX: Validate final point data
	if (!final_point->m_record || !final_point->m_record->m_player) {
		return;
	}

	math::VectorAngles(final_point->m_point - g_cl.m_shoot_pos, m_angle);

	// CRASH FIX: Safe SetData call
	try {
		final_point->m_record->SetData();
	}
	catch (...) {
		return;
	}

	if (!CheckHitchance(final_point, m_angle, m_hitchance, 128))
		return;

	if (g_cl.m_charged_ticks <= 0) {
		// limit choke to 1 if we can hit the target.
		g_cl.m_packet = g_csgo.m_cl->m_choked_commands >= 1;
	}

	if (!g_cl.m_weapon_fire || (g_csgo.m_cl->m_choked_commands <= 0 && g_cl.m_weapon_id != REVOLVER))
		return;

	if (g_menu.main.aimbot.auto_fire.get())
		g_cl.m_cmd->m_buttons |= IN_ATTACK;

	if (!(g_cl.m_cmd->m_buttons & IN_ATTACK))
		return;

	g_cl.m_cmd->m_tick = game::TIME_TO_TICKS(final_point->m_record->m_time + g_cl.m_lerp);
	g_cl.m_cmd->m_view_angles = m_angle - g_cl.m_local->m_aimPunchAngle() * g_csgo.weapon_recoil_scale->GetFloat();

	g_shots.OnShotFire(final_point);

	g_movement.m_done_retreating = false;
}

void Aimbot::ComputeHitchanceSeeds() {
	for (size_t i = 0; i < m_spread_seeds.size(); i++) {
		SpreadSeeds_t* seed = &m_spread_seeds[i];
		if (!seed)
			continue;

		g_csgo.RandomSeed(i);

		seed->a = g_csgo.RandomFloat(0.f, math::pi_2);
		seed->b = g_csgo.RandomFloat(0.f, 1.f);
		seed->c = g_csgo.RandomFloat(0.f, 1.f);
	}
}

float Aimbot::PointHitchance(PointData* point, const ang_t& angle, const size_t& max_seeds) const {
	static vec3_t     fwd, right, up, end;

	math::AngleVectors(angle, &fwd, &right, &up);

	size_t total_hits{ 0 };
	for (size_t i{ 0 }; i < max_seeds; i++) {
		end = g_cl.m_weapon->CalculateSpread(&m_spread_seeds.data()[i], g_input_pred.m_inaccuracy, g_input_pred.m_spread, false);
		end = g_cl.m_shoot_pos + ((fwd + (right * end.x) + (up * end.y)).normalized() * g_cl.m_weapon_info->m_range);

		CGameTrace trace;
		trace.m_fraction = 1.f;
		trace.m_startsolid = false;

		if (g_csgo.ClipRayToHitbox(Ray(g_cl.m_shoot_pos, end), point->m_bbox, point->m_record->m_bones[point->m_bbox->m_bone], &trace) == -1)
			continue;

		++total_hits;
	}

	return static_cast<float>(total_hits) / max_seeds;
}

bool Aimbot::CheckHitchance(PointData* point, const ang_t& angle, const float& chance, const size_t& max_seeds) {
	// we are gonna fire eitherway.
	if (g_cl.m_cmd->m_buttons & IN_ATTACK)
		return true;

	vec3_t fwd, right, up;
	math::AngleVectors(angle, &fwd, &right, &up);

	size_t total_hits{ 0 };
	size_t needed_hits{ static_cast<size_t>(max_seeds * chance / g_cl.m_weapon_info->m_bullets) };

	m_input.m_start = g_cl.m_shoot_pos;
	m_input.m_from = g_cl.m_local;
	m_input.m_target = point->m_record->m_player;
	m_input.m_pen = true;

	int tolerated_damage = std::max(std::min(m_minimum_damage, point->m_record->m_player->m_iHealth()) * (1.f - m_settings.damage_tolerance.get() * 0.01f), 1.f);

	for (size_t i{ 0 }; i < max_seeds; i++) {
		m_input.m_end = g_cl.m_weapon->CalculateSpread(&m_spread_seeds.data()[i], g_input_pred.m_inaccuracy, g_input_pred.m_spread, false);
		m_input.m_end = g_cl.m_shoot_pos + ((fwd + (right * m_input.m_end.x) + (up * m_input.m_end.y)).normalized() * g_cl.m_weapon_info->m_range);

		if (auto_wall::RunPenetration(&m_input).m_damage < tolerated_damage)
			continue;

		++total_hits;

		if (total_hits >= needed_hits)
			return true;
	}

	return false;
}

size_t Aimbot::CheckIntersection(PointData* point) {
	size_t hits{ 0 };
	vec3_t final = g_cl.m_shoot_pos + (point->m_point - g_cl.m_shoot_pos).normalized() * g_cl.m_weapon_info->m_range;

	for (size_t i = 0; i < point->m_record->m_safe_count; i++) {
		CGameTrace trace;
		trace.m_fraction = 1.f;
		trace.m_startsolid = false;

		if (g_csgo.ClipRayToHitbox(Ray(g_cl.m_shoot_pos, final), point->m_bbox, point->m_record->m_safe_bones[i].m_bones[point->m_bbox->m_bone], &trace) != -1)
			++hits;
	}

	return hits;
}

int Aimbot::GetMinimumIntersections(LagRecord* record) {
	switch (record->m_resolver_mode) {
		// we aint risking it.
	case RESOLVE_STAND_NO_DATA:
		if (m_settings.overlap.get(1))
			return 100;
		break;
	case RESOLVE_DISTORTION:
		if (m_settings.overlap.get(0))
			return 100;
		break;
	case RESOLVE_STAND_BALANCE2:
		if (m_settings.overlap.get(2))
			return 100;
		break;
	}

	return 0;
}