/*
 *
 * SOGV
 * sdl2/opengl/gltf based vita compatible game framework made in pure c
 * actually not
 *
 */

#ifndef SOGV_H
#define SOGV_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <SDL2/SDL.h>
#ifndef __vita__
    #include "glad/glad.h"
    #ifdef __APPLE__
        #include <OpenGL/gl.h>
    #else
        #include <GL/gl.h>
    #endif
#else
    #include <vitaGL.h>
#endif

#include "linmath.h"
#include "stb_image.h"

#define M_PI 3.14159265358979323846
#define MAX_BONE_INFLUENCE 4
#define MAX_BONES 100
#define ANALOG_DEADZONE 8000
#define SOGV_ATTR_POSITION_ID 0
#define SOGV_ATTR_NORMAL_ID 1
#define SOGV_ATTR_UV_ID 2
#define SOGV_ATTR_BONE_ID 3
#define SOGV_ATTR_WEIGHT_ID 4

typedef unsigned int uint;

typedef enum {FRONT, BACK, LEFT, RIGHT} direction;

typedef enum {DIFFUSE, SPECULAR} tex_type;

typedef struct sogv_vert {
    vec3 pos;
    vec3 normal;
    vec2 uv;
    float bone_info[MAX_BONE_INFLUENCE];
    float weights[MAX_BONE_INFLUENCE];
} sogv_vert;

typedef struct sogv_skel_node {
    char name[64];
    struct sogv_skel_node* children[MAX_BONES];
    vec3* pos_keys;
    quat* rot_keys;
    vec3* sca_keys;
    float* pos_key_times;
    float* rot_key_times;
    float* sca_key_times;
    size_t pos_keys_count;
    size_t rot_keys_count;
    size_t sca_keys_count;
    size_t child_count;
    int bone_idx;
} sogv_skel_node;

typedef struct sogv_skel_anim {
    sogv_skel_node* node;
    mat4x4 parent_mat;
    mat4x4* bone_mats;
    mat4x4* bone_anim_mats;
} sogv_skel_anim;

typedef struct sogv_mesh {
    sogv_vert* verts;
    uint* indices;
    size_t vert_count;
    size_t indice_count;
    size_t mat_idx;
    GLuint vao, vbo, ebo;
} sogv_mesh;

typedef struct sogv_model {
    sogv_mesh* meshes;
    GLuint* materials;
    mat4x4 bones[MAX_BONES];
    char bone_names[MAX_BONES][64];
    sogv_skel_node* root_node;
    float anim_dur;
    float anim_ticks;
    size_t mesh_count;
    size_t mat_count;
    size_t bone_count;
} sogv_model;

typedef struct sogv_cam {
    vec3 position;
    vec3 front;
    vec3 up;
    float yaw;
    float pitch;
    float mov_spd;
    float rot_spd;
    direction mov_dir;
    direction rot_dir;
    bool moving;
    bool rotating;
} sogv_cam;

typedef struct sogv_base {
    uint64_t start_count, end_count;
    uint64_t last_tick, current_tick;
    float performance, elapsed_ticks;
    SDL_Event sdl_event;
    SDL_Window* window;
    SDL_GLContext context;
    SDL_GameController** gamepads;
    bool running;
} sogv_base;

#define sogv_log(A) printf("[LOG]\t" A ".\n")
#define sogv_log_v(A, ...) printf("[LOG]\t" A ".\n", __VA_ARGS__)
#define sogv_die(A) printf("[DIE]\t" A ".\n"), exit(EXIT_FAILURE)
#define sogv_die_v(A, ...) printf("[DIE]\t" A ".\n", __VA_ARGS__), exit(EXIT_FAILURE)

#define sogv_parse_enum(e) case(e): out = #e; break;
#define sogv_deg_to_rad(ANGLE_DEG) ((ANGLE_DEG) * M_PI / 180.0)
#define sogv_rad_to_deg(ANGLE_RAD) ((ANGLE_RAD) * 180.0 / M_PI)

#define sogv_arr_resize(TYPE, ARRAY, SIZE)      \
{                                               \
    TYPE* new = realloc(ARRAY, SIZE);           \
    if(!new) sogv_die("Could not resize array");\
    ARRAY = new;                                \
}                                               \

char* sogv_read_file(const char* path);

sogv_base sogv_base_create(const char* title, int w, int h, const uint32_t sdl_flags);
#define sogv_base_loop_start(BASE)                                              \
{                                                                               \
    BASE.start_count = SDL_GetPerformanceCounter();                             \
    glClearColor(0.0f, 0.3f, 0.5f, 0.0f);                                       \
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                         \
    BASE.current_tick = SDL_GetTicks64();                                       \
    BASE.elapsed_ticks = (BASE.current_tick - BASE.last_tick) / 1000.0f;        \
}                                                                               \

#define sogv_base_loop_end(BASE)                                                                        \
{                                                                                                       \
    BASE.last_tick = BASE.current_tick;                                                                 \
    SDL_GL_SwapWindow(BASE.window);                                                                     \
    BASE.end_count = SDL_GetPerformanceCounter();                                                       \
    BASE.performance = (BASE.end_count - BASE.start_count) / (SDL_GetPerformanceFrequency() * 1000.0f); \
}                                                                                                       \

void sogv_base_clean(sogv_base* b);

void sogv_gl_check(const char* msg);
GLuint sogv_gl_shader_create(const char* vertex_path, const char* fragment_path);
#define sogv_gl_uniform_set_bool(SHADER, UNIFORM, VALUE) glUniform1i(glGetUniformLocation(SHADER, UNIFORM), (int)VALUE)
#define sogv_gl_uniform_set_int(SHADER, UNIFORM, VALUE) glUniform1i(glGetUniformLocation(SHADER, UNIFORM), VALUE)
#define sogv_gl_uniform_set_float(SHADER, UNIFORM, VALUE) glUniform1f(glGetUniformLocation(SHADER, UNIFORM), VALUE)
#define sogv_gl_uniform_set_float2(SHADER, UNIFORM, X, Y) glUniform2f(glGetUniformLocation(SHADER, UNIFORM), X, Y)
#define sogv_gl_uniform_set_float3(SHADER, UNIFORM, X, Y, Z) glUniform3f(glGetUniformLocation(SHADER, UNIFORM), X, Y, Z)
#define sogv_gl_uniform_set_float4(SHADER, UNIFORM, X, Y, Z, W) glUniform4f(glGetUniformLocation(SHADER, UNIFORM), X, Y, Z, W)
#define sogv_gl_uniform_set_vec2(SHADER, UNIFORM, VEC2) glUniform2fv(glGetUniformLocation(SHADER, UNIFORM), 1, &VEC2[0])
#define sogv_gl_uniform_set_vec3(SHADER, UNIFORM, VEC3) glUniform3fv(glGetUniformLocation(SHADER, UNIFORM), 1, &VEC3[0])
#define sogv_gl_uniform_set_vec4(SHADER, UNIFORM, VEC4) glUniform4fv(glGetUniformLocation(SHADER, UNIFORM), 1, &VEC4[0])
#define sogv_gl_uniform_set_mat4x4(SHADER, UNIFORM, MAT4X4) glUniformMatrix4fv(glGetUniformLocation(SHADER, UNIFORM), 1, GL_FALSE, &MAT4X4[0][0])
#define sogv_gl_uniform_set_mat4x4_v(SHADER, COUNT, UNIFORM, MAT4X4) glUniformMatrix4fv(glGetUniformLocation(SHADER, UNIFORM), COUNT, GL_FALSE, &MAT4X4[0][0])

#define sogv_gl_tex_parameterize(TYPE, WRAP, MIPMAP, FILTER)         \
{                                                               \
    glTexParameteri(TYPE, GL_TEXTURE_WRAP_S, WRAP);             \
    glTexParameteri(TYPE, GL_TEXTURE_WRAP_T, WRAP);             \
    glTexParameteri(TYPE, GL_TEXTURE_MIN_FILTER, MIPMAP);       \
    glTexParameteri(TYPE, GL_TEXTURE_MAG_FILTER, FILTER);       \
}                                                               \

sogv_model* sogv_model_create(const char* folder, const char* file);
void sogv_model_render(sogv_model* model);
void sogv_model_free(sogv_model* model);

void sogv_skel_animate(sogv_skel_node* node, float anim_time, mat4x4 parent_mat, mat4x4* bones, mat4x4* bone_anim_mats);

sogv_cam sogv_cam_create(const float pos_x, const float pos_y, const float pos_z, const float mov_spd, const float rot_spd);
void sogv_cam_handle_events(sogv_cam* cam, const SDL_Event e);
void sogv_cam_movement(sogv_cam* cam, const float ticks);

GLuint sogv_gl_stb_texture_create(const char* path);

#endif
