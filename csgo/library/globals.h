/*
 * originally written in C (2019), ported to C++ (17/09/2022)
 * i think code is easier to understand when it's written with C++.
 * 
 * what EC-CSGO open-source version lacks of(?)
 * - security features
 *
 */


#ifndef GLOBALS_H
#define GLOBALS_H


typedef struct {
	float x, y;
} vec2 ;

typedef struct {
	float x, y, z;
} vec3 ;

//
// debugging 
//
// #define DEBUG


#ifndef _KERNEL_MODE
#define DEBUG
#endif


#ifdef _KERNEL_MODE
extern "C" extern int _fltused;
#include <ntifs.h>
#else
#include <windows.h>
#endif

#ifdef DEBUG
#include <stdio.h>
#endif

#ifdef _KERNEL_MODE
	#define LOG DbgPrint
#else
	#define LOG printf
#endif

#ifdef _KERNEL_MODE
typedef unsigned __int8  BYTE;
typedef unsigned __int16 WORD;
typedef unsigned __int32 DWORD;
typedef unsigned __int64 QWORD;
typedef int BOOL;

extern PPHYSICAL_MEMORY_RANGE g_memory_range;
extern int g_memory_range_count;

inline int _strcmpi(const char* s1, const char* s2)
{	
	while(*s1 && (tolower(*s1) == tolower(*s2)))
	{
		s1++;
		s2++;
	}
	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}


#else
typedef unsigned __int64 QWORD;
#endif

#ifndef MATRIX_3X4
#define MATRIX_3X4
typedef float matrix3x4_t[3][4];
#endif

#define X_PI 3.14159f 
#define X_DIV 57.29577f
#define qabs(x) ((x) < 0 ? -(x) : (x))
#define qmax(a, b) (((a) > (b)) ? (a) : (b))
#define qmin(a, b) (((a) < (b)) ? (a) : (b))

namespace math
{
	inline float qsqrt(float x)
	{
		union
		{
			int i;
			float f;
		} u;

		u.f = x;
		u.i = (1 << 29) + (u.i >> 1) - (1 << 22);

		u.f = u.f + x / u.f;
		u.f = 0.25f * u.f + x / u.f;

		return u.f;
	}


	inline float qclamp(float x, float min, float max)
	{
		if (x < min) x = min;
		if (x > max) x = max;
		return x;
	}

	inline float qatan2(float y, float x)
	{
		float t0, t1, t3, t4;
		t3 = qabs(x);
		t1 = qabs(y);
		t0 = qmax(t3, t1);
		t1 = qmin(t3, t1);
		t3 = 1 / t0;
		t3 = t1 * t3;

		t4 = t3 * t3;
		t0 = -0.0134804f;
		t0 = t0 * t4 + 0.05747f;
		t0 = t0 * t4 - 0.12123f;
		t0 = t0 * t4 + 0.19563f;
		t0 = t0 * t4 - 0.33299f;
		t0 = t0 * t4 + 0.99999f;
		t3 = t0 * t3;

		t3 = (qabs(y) > qabs(x)) ? 1.57079f - t3 : t3;
		t3 = (x < 0) ? X_PI - t3 : t3;
		t3 = (y < 0) ? -t3 : t3;

		return t3;
	}

	inline float qatan(float x)
	{
		return qatan2(x, 1);
	}

	inline float qacos(float x)
	{
		int negate = (x < 0);
		x = qabs(x);
		float ret = -0.01872f;
		ret = ret * x;
		ret = ret + 0.07426f;
		ret = ret * x;
		ret = ret - 0.21211f;
		ret = ret * x;
		ret = ret + 1.57072f;
		ret = ret * qsqrt(1.0f - x);
		ret = ret - 2 * negate * ret;
		return negate * X_PI + ret;
	}


	#define RAD2DEG(x) ((float)(x) * (float)(180.f / 3.14159265358979323846f))
	#define DEG2RAD(x) ((float)(x) * (float)(3.14159265358979323846f / 180.f))
	inline double qpow(double a, double b)
	{
		double c = 1;
		for (int i = 0; i < b; i++)
			c *= a;
		return c;
	}

	inline double qfact(double x)
	{
		double ret = 1;
		for (int i = 1; i <= x; i++)
			ret *= i;
		return ret;
	}

	inline double qsin(double x)
	{
		double y = x;
		double s = -1;
		for (int i = 3; i <= 100; i += 2) {
			y += s * (qpow(x, (float)i) / qfact(i));
			s *= -1;
		}
		return y;
	}

	inline double qcos(double x)
	{
		double y = 1;
		double s = -1;
		for (int i = 2; i <= 100; i += 2) {
			y += s * (qpow(x, (float)i) / qfact(i));
			s *= -1;
		}
		return y;
	}

	inline double qtan(double x)
	{
		return (qsin(x) / qcos(x));
	}

	inline float qfloor(float x)
	{
		if (x >= 0.0f)
			return (float)((int)x);
		return (float)((int)x - 1);
	}

	inline float qfmodf(float a, float b)
	{
		return (a - b * qfloor(a / b));
	}

	inline void sincos(float radians, float* sine, float* cosine)
	{
		*sine = (float)qsin(radians);
		*cosine = (float)qcos(radians);
	}

	inline void angle_vec(vec3 angles, vec3* forward)
	{
		float sp, sy, cp, cy;
		sincos(DEG2RAD(angles.x), &sp, &cp);
		sincos(DEG2RAD(angles.y), &sy, &cy);
		forward->x = cp * cy;
		forward->y = cp * sy;
		forward->z = -sp;
	}

	inline float vec_dot(vec3 v0, vec3 v1)
	{
		return (v0.x * v1.x + v0.y * v1.y + v0.z * v1.z);
	}

	inline float vec_length(vec3 v)
	{
		return (v.x * v.x + v.y * v.y + v.z * v.z);
	}

	inline void vec_clamp(vec3* v)
	{
		if (v->x > 89.0f && v->x <= 180.0f) {
			v->x = 89.0f;
		}
		if (v->x > 180.0f) {
			v->x = v->x - 360.0f;
		}
		if (v->x < -89.0f) {
			v->x = -89.0f;
		}
		v->y = qfmodf(v->y + 180, 360) - 180;
		v->z = 0;
	}

	inline void vec_angles(vec3 forward, vec3* angles)
	{
		float tmp, yaw, pitch;

		if (forward.y == 0.f && forward.x == 0.f) {
			yaw = 0;
			if (forward.z > 0) {
				pitch = 270;
			}
			else {
				pitch = 90.f;
			}
		}
		else {
			yaw = (float)(qatan2(forward.y, forward.x) * 180.f / 3.14159265358979323846f);
			if (yaw < 0) {
				yaw += 360.f;
			}
			tmp = (float)qsqrt(forward.x * forward.x + forward.y * forward.y);
			pitch = (float)(qatan2(-forward.z, tmp) * 180.f / 3.14159265358979323846f);
			if (pitch < 0) {
				pitch += 360.f;
			}
		}
		angles->x = pitch;
		angles->y = yaw;
		angles->z = 0.f;
	}

	inline void vec_normalize(vec3* vec)
	{
		float radius;
		radius = 1.f / (float)(qsqrt(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z) + 1.192092896e-07f);
		vec->x *= radius, vec->y *= radius, vec->z *= radius;
	}

	inline float vec_length_sqrt(vec3 p0)
	{
		return (float)qsqrt(p0.x * p0.x + p0.y * p0.y + p0.z * p0.z);
	}

	inline vec3 vec_sub(vec3 p0, vec3 p1)
	{
		vec3 r;
		r.x = p0.x - p1.x;
		r.y = p0.y - p1.y;
		r.z = p0.z - p1.z;
		return r;
	}

	inline float vec_distance(vec3 p0, vec3 p1)
	{
		return vec_length_sqrt(vec_sub(p0, p1));
	}

	inline float get_fov_distance(vec3 vangle, vec3 angle, float distance)
	{
		vec3 a0, a1;
		angle_vec(vangle, &a0);
		angle_vec(angle, &a1);
		return RAD2DEG((qacos(vec_dot(a0, a1) / vec_length(a0))) * distance);
	}

	inline float get_fov(vec2 scrangles, vec3 aimangles)
	{
		vec3 delta;

		delta.x = aimangles.x - scrangles.x;
		delta.y = aimangles.y - scrangles.y;

		if (delta.x > 180)
			delta.x = 360 - delta.x;
		if (delta.x < 0)
			delta.x = -delta.x;

		delta.y = qfmodf(delta.y + 180, 360) - 180;
		if (delta.y < 0)
			delta.y = -delta.y;

		return qsqrt( (float)(qpow(delta.x, 2.0) + qpow(delta.y, 2.0)) );
	}

	inline vec3 vec_delta(vec3 p0, vec3 p1)
	{
		vec3  d;
		float l;
		d = vec_sub(p0, p1);
		l = (float)vec_length_sqrt(d);
		d.x /= l; d.y /= l; d.z /= l;
		return d;
	}


	inline vec3 vec_transform(vec3 p0, matrix3x4_t p1)
	{
		vec3 v;

		v.x = (p0.x * p1[0][0] + p0.y * p1[0][1] + p0.z * p1[0][2]) + p1[0][3];
		v.y = (p0.x * p1[1][0] + p0.y * p1[1][1] + p0.z * p1[1][2]) + p1[1][3];
		v.z = (p0.x * p1[2][0] + p0.y * p1[2][1] + p0.z * p1[2][2]) + p1[2][3];
		return v;
	}

	inline vec3 vec_atd(vec3 vangle)
	{
		double y[2], p[2];
		vangle.x *= (3.14159265358979323846f / 180.f);
		vangle.y *= (3.14159265358979323846f / 180.f);
		y[0]     = qsin(vangle.y), y[1] = qcos(vangle.y);
		p[0]     = qsin(vangle.x), p[1] = qcos(vangle.x);
		vangle.x = (float)(p[1] * y[1]);
		vangle.y = (float)(p[1] * y[0]);
		vangle.z = (float)-p[0];
		return vangle;
	}

	inline BOOL vec_min_max(vec3 eye, vec3 dir, vec3 min, vec3 max, float radius)
	{
	    vec3     delta;
	    int      i;
	    vec3     q;
	    float    v;


	    //
	    // original maths by superdoc1234
	    //
	    delta = vec_delta(max, min);
	    for ( i = 0; i < vec_distance(min, max); i++ ) {
		q.x = min.x + delta.x * (float)i - eye.x;
		q.y = min.y + delta.y * (float)i - eye.y;
		q.z = min.z + delta.z * (float)i - eye.z;
		if ((v = vec_dot(q, dir)) < 1.0f) {
		    return 0;
		}
		v = radius * radius - (vec_length(q) - v * v);

		if ( v <= -100.f ) {
		    return 0;
		}
		if (v >= 1.19209290E-07F) {
		    return 1;
		}
	    }
	    return 0;
	}
}

typedef void  *vm_handle;

enum class VM_MODULE_TYPE {
	Full = 1,
	CodeSectionsOnly = 2,
	Raw = 3 // used for dump to file
} ;

namespace vm
{
	BOOL      process_exists(PCSTR process_name);
	vm_handle open_process(PCSTR process_name);
	vm_handle open_process_ex(PCSTR process_name, PCSTR dll_name);
	vm_handle open_process_by_module_name(PCSTR dll_name);
	void      close(vm_handle process);
	BOOL      running(vm_handle process);

	BOOL      read(vm_handle process, QWORD address, PVOID buffer, QWORD length);
	BYTE      read_i8(vm_handle process, QWORD address);
	WORD      read_i16(vm_handle process, QWORD address);
	DWORD     read_i32(vm_handle process, QWORD address);
	QWORD     read_i64(vm_handle process, QWORD address);
	float     read_float(vm_handle process, QWORD address);

	BOOL      write(vm_handle process, QWORD address, PVOID buffer, QWORD length);
	BOOL      write_i8(vm_handle process, QWORD address, BYTE value);
	BOOL      write_i16(vm_handle process, QWORD address, WORD value);
	BOOL      write_i32(vm_handle process, QWORD address, DWORD value);
	BOOL      write_i64(vm_handle process, QWORD address, QWORD value);
	BOOL      write_float(vm_handle process, QWORD address, float value);

	QWORD     get_relative_address(vm_handle process, QWORD instruction, DWORD offset, DWORD instruction_size);
	QWORD     get_peb(vm_handle process);
	QWORD     get_wow64_process(vm_handle process);

	QWORD     get_module(vm_handle process, PCSTR dll_name);
	QWORD     get_module_export(vm_handle process, QWORD base, PCSTR export_name);

	PVOID     dump_module(vm_handle process, QWORD base, VM_MODULE_TYPE module_type);
	void      free_module(PVOID dump_module);
	QWORD     scan_pattern(PVOID dumped_module, PCSTR pattern, PCSTR mask, QWORD length);
}


typedef DWORD C_Player;
typedef DWORD C_TeamList;
typedef DWORD C_Team;
typedef DWORD C_PlayerList;

namespace cs
{
	enum class WEAPON_CLASS {
		Invalid = 0,
		Knife = 1,
		Grenade = 2,
		Pistol = 3,
		Sniper = 4,
		Rifle = 5,
	} ;

	BOOL  running(void);

	namespace teams
	{
		C_Player     get_local_player(void);
		C_TeamList   get_team_list(void);
		int          get_team_count(void);
		C_Team       get_team(C_TeamList team_list, int index);
		int          get_team_num(C_Team team);
		int          get_player_count(C_Team team);
		C_PlayerList get_player_list(C_Team team);
		C_Player     get_player(C_PlayerList player_list, int index);
		BOOL         contains_player(C_PlayerList player_list, int player_count, int index);
	}

	namespace engine
	{
		vec2  get_viewangles(void);
		float get_sensitivity(void);
		BOOL  is_gamemode_ffa(void);
		DWORD get_current_tick(void);
	}

	namespace input
	{
		BOOL  get_button_state(DWORD button);
		void  mouse_move(int x, int y);
	}

	//
	// use it with teams
	//
	namespace player
	{

		BOOL         is_valid(C_Player player_address);
		BOOL         is_visible(C_Player local_player, C_Player player);
		BOOL         is_defusing(C_Player player_address);
		BOOL         has_defuser(C_Player player_address);
		int          get_player_id(C_Player player_address);
		int          get_crosshair_id(C_Player player_address);
		BOOL         get_dormant(C_Player player_address);
		int          get_life_state(C_Player player_address);
		int          get_health(C_Player player_address);
		int          get_shots_fired(C_Player player_address);
		vec2         get_vec_punch(C_Player player_address);
		int          get_fov(C_Player player_address);
		DWORD        get_weapon_handle(C_Player player_address);
		WEAPON_CLASS get_weapon_class(C_Player player_address);
		vec3         get_eye_position(C_Player player_address);
		BOOL         get_bone_position(C_Player player_address, int bone_index, matrix3x4_t *matrix);
	}
}

namespace input
{
	void mouse_move(int x, int y);
	void mouse1_down(void);
	void mouse1_up(void);
}

namespace features
{
	void run(void);
}

namespace csgo
{
	inline void run(void)
	{
		if (cs::running())
		{
			features::run();
		}
	}
}

namespace config
{
	extern BOOL  rcs;
	extern DWORD aimbot_button;
	extern float aimbot_fov;
	extern float aimbot_smooth;
	extern BOOL  aimbot_visibility_check;
	extern DWORD triggerbot_button;
}

#endif // GLOBALS_H

