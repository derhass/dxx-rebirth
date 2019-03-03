/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * object system definitions
 *
 */

#pragma once

#include <type_traits>
#include "dsx-ns.h"
#ifdef dsx

#include "pstypes.h"
#include "vecmat.h"
#include "aistruct.h"
#include "polyobj.h"
#include "laser.h"

#ifdef __cplusplus
#include <cassert>
#include <cstdint>
#include "dxxsconf.h"
#include "compiler-array.h"
#include "valptridx.h"
#include "objnum.h"
#include "fwd-segment.h"
#include <vector>
#include <stdexcept>
#include "fwd-object.h"
#include "weapon.h"
#include "powerup.h"
#include "compiler-integer_sequence.h"
#include "compiler-poison.h"
#include "player-flags.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "escort.h"
#endif

namespace dcx {

// Object types
enum object_type_t : uint8_t
{
	OBJ_NONE	= 255, // unused object
	OBJ_WALL	= 0,   // A wall... not really an object, but used for collisions
	OBJ_FIREBALL	= 1,   // a fireball, part of an explosion
	OBJ_ROBOT	= 2,   // an evil enemy
	OBJ_HOSTAGE	= 3,   // a hostage you need to rescue
	OBJ_PLAYER	= 4,   // the player on the console
	OBJ_WEAPON	= 5,   // a laser, missile, etc
	OBJ_CAMERA	= 6,   // a camera to slew around with
	OBJ_POWERUP	= 7,   // a powerup you can pick up
	OBJ_DEBRIS	= 8,   // a piece of robot
	OBJ_CNTRLCEN	= 9,   // the control center
	OBJ_CLUTTER	= 11,  // misc objects
	OBJ_GHOST	= 12,  // what the player turns into when dead
	OBJ_LIGHT	= 13,  // a light source, & not much else
	OBJ_COOP	= 14,  // a cooperative player object.
	OBJ_MARKER	= 15,  // a map marker
};

enum movement_type_t : uint8_t
{
	MT_NONE = 0,   // doesn't move
	MT_PHYSICS = 1,   // moves by physics
	MT_SPINNING = 3,   // this object doesn't move, just sits and spins
};

}

namespace dsx {

/*
 * STRUCTURES
 */

struct reactor_static {
	/* Location of the gun on the reactor object */
	array<vms_vector, MAX_CONTROLCEN_GUNS>	gun_pos,
	/* Orientation of the gun on the reactor object */
		gun_dir;
};

struct player_info
{
	fix     energy;                 // Amount of energy remaining.
	fix     homing_object_dist;     // Distance of nearest homing object.
	fix Fusion_charge;
#if defined(DXX_BUILD_DESCENT_II)
	fix Omega_charge;
	fix Omega_recharge_delay;
#endif
	player_flags powerup_flags;
	objnum_t   killer_objnum;          // Who killed me.... (-1 if no one)
	uint16_t vulcan_ammo;
#if defined(DXX_BUILD_DESCENT_I)
	using primary_weapon_flag_type = uint8_t;
#elif defined(DXX_BUILD_DESCENT_II)
	using primary_weapon_flag_type = uint16_t;
#endif
	primary_weapon_flag_type primary_weapon_flags;
	bool Player_eggs_dropped;
	bool FakingInvul;
	bool lavafall_hiss_playing;
	uint8_t missile_gun;
	player_selected_weapon<primary_weapon_index_t> Primary_weapon;
	player_selected_weapon<secondary_weapon_index_t> Secondary_weapon;
	stored_laser_level laser_level;
	array<uint8_t, MAX_SECONDARY_WEAPONS>  secondary_ammo; // How much ammo of each type.
#if defined(DXX_BUILD_DESCENT_II)
	uint8_t Primary_last_was_super;
	uint8_t Secondary_last_was_super;
#endif
	int16_t net_killed_total;		// Number of times killed total
	int16_t net_kills_total;		// Number of net kills total
	int16_t KillGoalCount;			// Num of players killed this level
	union {
		struct {
			int score;				// Current score.
			int last_score;			// Score at beginning of current level.
			uint16_t hostages_rescued_total; // Total number of hostages rescued.
			uint8_t hostages_on_board;
		} mission;
		struct {
			uint8_t orbs;
		} hoard;
	};
	enum
	{
		max_hoard_orbs = 12,
	};
	fix64   cloak_time;             // Time cloaked
	fix64   invulnerable_time;      // Time invulnerable
	fix64 Next_flare_fire_time;
	fix64 Next_laser_fire_time;
	fix64 Next_missile_fire_time;
	fix64 Last_bumped_local_player;
	fix64 Auto_fire_fusion_cannon_time;
};

}

namespace dcx {

// A compressed form for sending crucial data
struct shortpos
{
	sbyte   bytemat[9];
	short   xo,yo,zo;
	short   segment;
	short   velx, vely, velz;
} __pack__;

// Another compressed form for object position, velocity, orientation and rotvel using quaternion
struct quaternionpos : prohibit_void_ptr<quaternionpos>
{
	using packed_size = std::integral_constant<std::size_t, sizeof(vms_quaternion) + sizeof(segnum_t) + (sizeof(vms_vector) * 3)>;
	vms_quaternion orient;
	vms_vector pos;
	segnum_t segment;
	vms_vector vel;
	vms_vector rotvel;
};

// information for physics sim for an object
struct physics_info : prohibit_void_ptr<physics_info>
{
	vms_vector  velocity;   // velocity vector of this object
	vms_vector  thrust;     // constant force applied to this object
	fix         mass;       // the mass of this object
	fix         drag;       // how fast this slows down
	vms_vector  rotvel;     // rotational velecity (angles)
	vms_vector  rotthrust;  // rotational acceleration
	fixang      turnroll;   // rotation caused by turn banking
	ushort      flags;      // misc physics flags
};

struct physics_info_rw
{
	vms_vector  velocity;   // velocity vector of this object
	vms_vector  thrust;     // constant force applied to this object
	fix         mass;       // the mass of this object
	fix         drag;       // how fast this slows down
	fix         obsolete_brakes;     // how much brakes applied
	vms_vector  rotvel;     // rotational velecity (angles)
	vms_vector  rotthrust;  // rotational acceleration
	fixang      turnroll;   // rotation caused by turn banking
	ushort      flags;      // misc physics flags
} __pack__;

// stuctures for different kinds of simulation

struct laser_parent
{
	int16_t parent_type = {};        // The type of the parent of this object
	objnum_t parent_num = {};         // The object's parent's number
	object_signature_t parent_signature = {};   // The object's parent's signature...
};

struct laser_info : prohibit_void_ptr<laser_info>, laser_parent
{
	fix64 creation_time = 0;      // Absolute time of creation.
	/* hitobj_pos specifies the next position to which a value should be
	 * written.  That position may have a defined value if the array has
	 * wrapped, but should be treated as write-only in the general case.
	 *
	 * hitobj_count tells how many elements in hitobj_values[] are
	 * valid.  Its valid values are [0, hitobj_values.size()].  When
	 * hitobj_count == hitobj_values.size(), hitobj_pos wraps around and
	 * begins erasing the oldest elements first.
	 */
	uint8_t hitobj_pos = 0, hitobj_count = 0;
	std::array<objnum_t, 83> hitobj_values = {};
	objnum_t track_goal = 0;         // Object this object is tracking.
	fix multiplier = 0;         // Power if this is a fusion bolt (or other super weapon to be added).
	uint_fast8_t test_set_hitobj(const vcobjidx_t o);
	uint_fast8_t test_hitobj(const vcobjidx_t o) const;
	icobjidx_t get_last_hitobj() const
	{
		if (!hitobj_count)
			/* If no elements, return object_none */
			return object_none;
		/* Return the most recently written element.  `hitobj_pos`
		 * indicates the element to write next, so return
		 * hitobj_values[hitobj_pos - 1].  When hitobj_pos == 0, the
		 * most recently written element is at the end of the array, not
		 * before the beginning of the array.
		 */
		if (!hitobj_pos)
			return hitobj_values.back();
		return hitobj_values[hitobj_pos - 1];
	}
	void clear_hitobj()
	{
		hitobj_pos = hitobj_count = 0;
	}
	void reset_hitobj(const icobjidx_t o)
	{
		if (o == object_none)
		{
			/* Adding object_none to the array is harmless, since
			 * get_last_hitobj can return object_none for empty arrays.
			 * However, by filtering it here (which is called only when
			 * loading data into new objects), test_hitobj (which is
			 * called every time the object strikes a potential target)
			 * will not need to read a slot that is guaranteed not to
			 * match.
			 */
			clear_hitobj();
			return;
		}
		/* Assume caller already poisoned the unused array elements */
		hitobj_pos = hitobj_count = 1;
		hitobj_values[0] = o;
	}
};

// Same as above but structure Savegames/Multiplayer objects expect
struct laser_info_rw
{
	short   parent_type;        // The type of the parent of this object
	short   parent_num;         // The object's parent's number
	int     parent_signature;   // The object's parent's signature...
	fix     creation_time;      // Absolute time of creation.
	short   last_hitobj;        // For persistent weapons (survive object collision), object it most recently hit.
	short   track_goal;         // Object this object is tracking.
	fix     multiplier;         // Power if this is a fusion bolt (or other super weapon to be added).
} __pack__;

struct explosion_info : prohibit_void_ptr<explosion_info>
{
    fix     spawn_time;         // when lifeleft is < this, spawn another
    fix     delete_time;        // when to delete object
    objnum_t   delete_objnum;      // and what object to delete
    objnum_t   attach_parent;      // explosion is attached to this object
    objnum_t   prev_attach;        // previous explosion in attach list
    objnum_t   next_attach;        // next explosion in attach list
};

struct explosion_info_rw
{
    fix     spawn_time;         // when lifeleft is < this, spawn another
    fix     delete_time;        // when to delete object
    short   delete_objnum;      // and what object to delete
    short   attach_parent;      // explosion is attached to this object
    short   prev_attach;        // previous explosion in attach list
    short   next_attach;        // next explosion in attach list
} __pack__;

struct light_info : prohibit_void_ptr<light_info>
{
    fix     intensity;          // how bright the light is
};

struct light_info_rw
{
    fix     intensity;          // how bright the light is
} __pack__;

struct powerup_info : prohibit_void_ptr<powerup_info>
{
	int     count;          // how many/much we pick up (vulcan cannon only?)
	int     flags;          // spat by player?
	fix64   creation_time;  // Absolute time of creation.
};

}

namespace dsx {

struct powerup_info_rw
{
	int     count;          // how many/much we pick up (vulcan cannon only?)
#if defined(DXX_BUILD_DESCENT_II)
// Same as above but structure Savegames/Multiplayer objects expect
	fix     creation_time;  // Absolute time of creation.
	int     flags;          // spat by player?
#endif
} __pack__;

}

namespace dcx {

struct vclip_info : prohibit_void_ptr<vclip_info>
{
	int     vclip_num;
	fix     frametime;
	uint8_t framenum;
};

struct vclip_info_rw
{
	int     vclip_num;
	fix     frametime;
	sbyte   framenum;
} __pack__;

// structures for different kinds of rendering

struct polyobj_info : prohibit_void_ptr<polyobj_info>
{
	int     model_num;          // which polygon model
	array<vms_angvec, MAX_SUBMODELS> anim_angles; // angles for each subobject
	int     subobj_flags;       // specify which subobjs to draw
	int     tmap_override;      // if this is not -1, map all face to this
	int     alt_textures;       // if not -1, use these textures instead
};

struct polyobj_info_rw
{
	int     model_num;          // which polygon model
	vms_angvec anim_angles[MAX_SUBMODELS]; // angles for each subobject
	int     subobj_flags;       // specify which subobjs to draw
	int     tmap_override;      // if this is not -1, map all face to this
	int     alt_textures;       // if not -1, use these textures instead
} __pack__;

struct object_base
{
	object_signature_t signature;
	object_type_t   type;           // what type of object this is... robot, weapon, hostage, powerup, fireball
	ubyte   id;             // which form of object...which powerup, robot, etc.
	objnum_t   next,prev;      // id of next and previous connected object in Objects, -1 = no connection
	ubyte   control_type;   // how this object is controlled
	movement_type_t   movement_type;  // how this object moves
	ubyte   render_type;    // how this object renders
	ubyte   flags;          // misc flags
	segnum_t   segnum;         // segment number containing object
	objnum_t   attached_obj;   // number of attached fireball object
	vms_vector pos;         // absolute x,y,z coordinate of center of object
	vms_matrix orient;      // orientation of object in world
	fix     size;           // 3d size of object - for collision detection
	fix     shields;        // Starts at maximum, when <0, object dies..
	vms_vector last_pos;    // where object was last frame
	sbyte   contains_type;  // Type of object this object contains (eg, spider contains powerup)
	sbyte   contains_id;    // ID of object this object contains (eg, id = blue type = key)
	sbyte   contains_count; // number of objects of type:id this object contains
	sbyte   matcen_creator; // Materialization center that created this object, high bit set if matcen-created
	fix     lifeleft;       // how long until goes away, or 7fff if immortal
	// -- Removed, MK, 10/16/95, using lifeleft instead: int     lightlevel;

	// movement info, determined by MOVEMENT_TYPE
	union movement_info {
		physics_info phys_info; // a physics object
		vms_vector   spin_rate; // for spinning objects
		constexpr movement_info() :
			phys_info{}
		{
			static_assert(sizeof(phys_info) == sizeof(*this), "insufficient initialization");
		}
	} mtype;

	// render info, determined by RENDER_TYPE
	union render_info {
		struct polyobj_info    pobj_info;      // polygon model
		struct vclip_info      vclip_info;     // vclip
		constexpr render_info() :
			pobj_info{}
		{
			static_assert(sizeof(pobj_info) == sizeof(*this), "insufficient initialization");
		}
	} rtype;
};

}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
struct laser_info : public ::dcx::laser_info
{
	fix64 last_afterburner_time = 0;	//	Time at which this object last created afterburner blobs.
};
#endif

struct object : public ::dcx::object_base
{
	// control info, determined by CONTROL_TYPE
	union control_info {
		constexpr control_info() :
			ai_info{}
		{
			static_assert(sizeof(ai_info) == sizeof(*this), "insufficient initialization");
		}
		struct laser_info      laser_info;
		struct explosion_info  expl_info;      // NOTE: debris uses this also
		struct light_info      light_info;     // why put this here?  Didn't know what else to do with it.
		struct powerup_info    powerup_info;
		struct ai_static       ai_info;
		struct reactor_static  reactor_info;
		struct player_info     player_info;
	} ctype;
};

}

namespace dcx {

// Same as above but structure Savegames/Multiplayer objects expect
struct object_rw
{
	int     signature;      // Every object ever has a unique signature...
	ubyte   type;           // what type of object this is... robot, weapon, hostage, powerup, fireball
	ubyte   id;             // which form of object...which powerup, robot, etc.
	short   next,prev;      // id of next and previous connected object in Objects, -1 = no connection
	ubyte   control_type;   // how this object is controlled
	ubyte   movement_type;  // how this object moves
	ubyte   render_type;    // how this object renders
	ubyte   flags;          // misc flags
	short   segnum;         // segment number containing object
	short   attached_obj;   // number of attached fireball object
	vms_vector pos;         // absolute x,y,z coordinate of center of object
	vms_matrix orient;      // orientation of object in world
	fix     size;           // 3d size of object - for collision detection
	fix     shields;        // Starts at maximum, when <0, object dies..
	vms_vector last_pos;    // where object was last frame
	sbyte   contains_type;  // Type of object this object contains (eg, spider contains powerup)
	sbyte   contains_id;    // ID of object this object contains (eg, id = blue type = key)
	sbyte   contains_count; // number of objects of type:id this object contains
	sbyte   matcen_creator; // Materialization center that created this object, high bit set if matcen-created
	fix     lifeleft;       // how long until goes away, or 7fff if immortal
	// -- Removed, MK, 10/16/95, using lifeleft instead: int     lightlevel;

	// movement info, determined by MOVEMENT_TYPE
	union {
		physics_info_rw phys_info; // a physics object
		vms_vector   spin_rate; // for spinning objects
	} __pack__ mtype ;

	// control info, determined by CONTROL_TYPE
	union {
		laser_info_rw   laser_info;
		explosion_info_rw  expl_info;      // NOTE: debris uses this also
		ai_static_rw    ai_info;
		light_info_rw      light_info;     // why put this here?  Didn't know what else to do with it.
		powerup_info_rw powerup_info;
	} __pack__ ctype ;

	// render info, determined by RENDER_TYPE
	union {
		polyobj_info_rw    pobj_info;      // polygon model
		vclip_info_rw      vclip_info;     // vclip
	} __pack__ rtype;
} __pack__;

struct obj_position
{
	vms_vector  pos;        // absolute x,y,z coordinate of center of object
	vms_matrix  orient;     // orientation of object in world
	segnum_t       segnum;     // segment number containing object
};

#define set_object_type(O,T)	\
	( DXX_BEGIN_COMPOUND_STATEMENT {	\
		object_base &dxx_object_type_ref = (O);	\
		const uint8_t &dxx_object_type_value = (T);	\
		assert(	\
			dxx_object_type_value == OBJ_NONE ||	\
			dxx_object_type_value == OBJ_FIREBALL ||	\
			dxx_object_type_value == OBJ_ROBOT ||	\
			dxx_object_type_value == OBJ_HOSTAGE ||	\
			dxx_object_type_value == OBJ_PLAYER ||	\
			dxx_object_type_value == OBJ_WEAPON ||	\
			dxx_object_type_value == OBJ_CAMERA ||	\
			dxx_object_type_value == OBJ_POWERUP ||	\
			dxx_object_type_value == OBJ_DEBRIS ||	\
			dxx_object_type_value == OBJ_CNTRLCEN ||	\
			dxx_object_type_value == OBJ_CLUTTER ||	\
			dxx_object_type_value == OBJ_GHOST ||	\
			dxx_object_type_value == OBJ_LIGHT ||	\
			dxx_object_type_value == OBJ_COOP ||	\
			dxx_object_type_value == OBJ_MARKER	\
		);	\
		dxx_object_type_ref.type = static_cast<object_type_t>(dxx_object_type_value);	\
	} DXX_END_COMPOUND_STATEMENT )

#define set_object_movement_type(O,T)	\
	( DXX_BEGIN_COMPOUND_STATEMENT {	\
		object_base &dxx_object_movement_type_ref = (O);	\
		const uint8_t &dxx_object_movement_type_value = (T);	\
		assert(	\
			dxx_object_movement_type_value == MT_NONE ||	\
			dxx_object_movement_type_value == MT_PHYSICS ||	\
			dxx_object_movement_type_value == MT_SPINNING	\
		);	\
		dxx_object_movement_type_ref.movement_type = static_cast<movement_type_t>(dxx_object_movement_type_value);	\
	} DXX_END_COMPOUND_STATEMENT )

template <typename T, std::size_t... N>
constexpr array<T, sizeof...(N)> init_object_number_array(index_sequence<N...>)
{
	return {{((void)N, object_none)...}};
}

template <typename T, std::size_t N>
struct object_number_array : array<T, N>
{
	constexpr object_number_array() :
		array<T, N>(init_object_number_array<T>(make_tree_index_sequence<N>()))
	{
	}
};

}

#define Highest_object_index (Objects.get_count() - 1)

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
struct d_unique_buddy_state
{
	icobjidx_t Buddy_objnum = object_none;
	icobjidx_t Escort_goal_index = object_none;
	uint8_t Buddy_allowed_to_talk;
	uint8_t Buddy_messages_suppressed;
	uint8_t Buddy_gave_hint_count;
	uint8_t Looking_for_marker;
	int Last_buddy_key;
	int Last_buddy_polish_path_tick;
	escort_goal_t Escort_goal_object;
	escort_goal_t Escort_special_goal;
	fix64 Buddy_sorry_time;
	fix64 Buddy_last_seen_player;
	fix64 Buddy_last_missile_time;
	fix64 Last_time_buddy_gave_hint;
	fix64 Last_come_back_message_time;
	fix64 Escort_last_path_created;
	fix64 Buddy_last_player_path_created;
};

class d_guided_missile_indices : object_number_array<imobjidx_t, MAX_PLAYERS>
{
	template <typename R, typename F>
		R get_player_active_guided_missile_tmpl(F &fvcobj, unsigned pnum) const;
	static bool debug_check_current_object(const object_base &);
public:
	imobjidx_t get_player_active_guided_missile(unsigned pnum) const;
	imobjptr_t get_player_active_guided_missile(fvmobjptr &vmobjptr, unsigned pnum) const;
	imobjptridx_t get_player_active_guided_missile(fvmobjptridx &vmobjptridx, unsigned pnum) const;
	void set_player_active_guided_missile(vmobjidx_t, unsigned pnum);
	void clear_player_active_guided_missile(unsigned pnum);
};
#endif

struct d_level_unique_object_state
{
	unsigned num_objects = 0;
	unsigned Debris_object_count = 0;
#if defined(DXX_BUILD_DESCENT_II)
	d_unique_buddy_state BuddyState;
	d_guided_missile_indices Guided_missile;
#endif
	object_number_array<imobjidx_t, MAX_OBJECTS> free_obj_list;
	object_array Objects;
	auto &get_objects()
	{
		return Objects;
	}
	const auto &get_objects() const
	{
		return Objects;
	}
};

extern d_level_unique_object_state LevelUniqueObjectState;

static inline powerup_type_t get_powerup_id(const object_base &o)
{
	return static_cast<powerup_type_t>(o.id);
}

static inline weapon_id_type get_weapon_id(const object_base &o)
{
	return static_cast<weapon_id_type>(o.id);
}

#if defined(DXX_BUILD_DESCENT_II)
static inline uint8_t get_marker_id(const object_base &o)
{
	return o.id;
}
#endif

void set_powerup_id(const d_powerup_info_array &Powerup_info, const d_vclip_array &Vclip, object_base &o, powerup_type_t id);

static inline void set_weapon_id(object_base &o, weapon_id_type id)
{
	o.id = static_cast<uint8_t>(id);
}

}

namespace dcx {

static inline uint8_t get_player_id(const object_base &o)
{
	return o.id;
}

static inline uint8_t get_reactor_id(const object_base &o)
{
	return o.id;
}

static inline uint8_t get_fireball_id(const object_base &o)
{
	return o.id;
}

static inline uint8_t get_robot_id(const object_base &o)
{
	return o.id;
}

static inline void set_player_id(object_base &o, const uint8_t id)
{
	o.id = id;
}

static inline void set_reactor_id(object_base &o, const uint8_t id)
{
	o.id = id;
}

static inline void set_robot_id(object_base &o, const uint8_t id)
{
	o.id = id;
}

void check_warn_object_type(const object_base &, object_type_t, const char *file, unsigned line);
#define get_player_id(O)	(check_warn_object_type(O, OBJ_PLAYER, __FILE__, __LINE__), get_player_id(O))
#define get_powerup_id(O)	(check_warn_object_type(O, OBJ_POWERUP, __FILE__, __LINE__), get_powerup_id(O))
#define get_reactor_id(O)	(check_warn_object_type(O, OBJ_CNTRLCEN, __FILE__, __LINE__), get_reactor_id(O))
#define get_ghost_id(O)	(check_warn_object_type(O, OBJ_GHOST, __FILE__, __LINE__), (get_player_id)(O))
#define get_fireball_id(O)	(check_warn_object_type(O, OBJ_FIREBALL, __FILE__, __LINE__), get_fireball_id(O))
#define get_robot_id(O)	(check_warn_object_type(O, OBJ_ROBOT, __FILE__, __LINE__), get_robot_id(O))
#define get_weapon_id(O)	(check_warn_object_type(O, OBJ_WEAPON, __FILE__, __LINE__), get_weapon_id(O))
#if defined(DXX_BUILD_DESCENT_II)
#define get_marker_id(O)	(check_warn_object_type(O, OBJ_MARKER, __FILE__, __LINE__), get_marker_id(O))
#endif
#define set_player_id(O,I)	(check_warn_object_type(O, OBJ_PLAYER, __FILE__, __LINE__), set_player_id(O, I))
#define set_reactor_id(O,I)	(check_warn_object_type(O, OBJ_CNTRLCEN, __FILE__, __LINE__), set_reactor_id(O, I))
#define set_robot_id(O,I)	(check_warn_object_type(O, OBJ_ROBOT, __FILE__, __LINE__), set_robot_id(O, I))
#define set_weapon_id(O,I)	(check_warn_object_type(O, OBJ_WEAPON, __FILE__, __LINE__), set_weapon_id(O, I))
#ifdef DXX_CONSTANT_TRUE
#define check_warn_object_type(O,T,F,L)	\
	( DXX_BEGIN_COMPOUND_STATEMENT {	\
		const object_base &dxx_check_warn_o = (O);	\
		const auto dxx_check_warn_actual_type = dxx_check_warn_o.type;	\
		const auto dxx_check_warn_expected_type = (T);	\
		/* If the type is always right, omit the runtime check. */	\
		DXX_CONSTANT_TRUE(dxx_check_warn_actual_type == dxx_check_warn_expected_type) || (	\
			/* If the type is always wrong, force a compile-time error. */	\
			DXX_CONSTANT_TRUE(dxx_check_warn_actual_type != dxx_check_warn_expected_type)	\
			? DXX_ALWAYS_ERROR_FUNCTION(dxx_error_object_type_mismatch, "object type mismatch")	\
			: (	\
				check_warn_object_type(dxx_check_warn_o, dxx_check_warn_expected_type, F, L)	\
			)	\
		, 0);	\
	} DXX_END_COMPOUND_STATEMENT )
#endif

}
#endif

#endif
