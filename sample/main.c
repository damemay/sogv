#include <stdlib.h>
#include <sogv.h>

#define WIDTH                           1280
#define HEIGHT                          720
#define FOV                             90

#ifdef __vita__
extern unsigned int _newlib_heap_size_user = 300*1024*1024;
#endif

int main() {
    sogv_base game = sogv_base_create("newgl", WIDTH, HEIGHT,
            SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    stbi_set_flip_vertically_on_load(true);

    sogv_model* mod = sogv_model_create("../res/models/animation2/", "untitled.gltf");
    sogv_model* mod2 = sogv_model_create("../res/models/static/", "untitled.gltf");

    sogv_cam cam = sogv_cam_create(0.0f, 0.0f, 3.0f, 2.5f, 50.0f);

    mat4x4 proj;
    mat4x4_perspective(proj, sogv_deg_to_rad(FOV), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);

    GLuint shader = sogv_gl_shader_create("../res/shaders/normal_shader.vert",
            "../res/shaders/normal_shader.frag");
    GLuint shader2 = sogv_gl_shader_create("/home/mar/newgl/gltf-loading/res/shaders/normal_shader.vert",
            "/home/mar/newgl/gltf-loading/res/shaders/normal_shader.frag");
    glUseProgram(shader);
    
    /*
    sogv_gl_uniform_set_float3(shader, "light.direction", -0.2f, -1.0f, -0.3f);
    sogv_gl_uniform_set_float3(shader, "light.ambient", 0.2f, 0.2f, 0.2f);
    sogv_gl_uniform_set_float3(shader, "light.diffuse", 0.5f, 0.5f, 0.5f);
    sogv_gl_uniform_set_float3(shader, "light.specular", 1.0f, 1.0f, 1.0f);

    sogv_gl_uniform_set_float3(shader, "mat.diffuse", 0.5f, 0.5f, 0.5f);
    sogv_gl_uniform_set_float3(shader, "mat.specular", 0.7f, 0.7f, 0.7f);
    sogv_gl_uniform_set_float(shader, "mat.shininess", 32.0f);
    */

    mat4x4 anim[MAX_BONES];
    for(size_t i=0; i<MAX_BONES; ++i){
        mat4x4_identity(anim[i]);
    }
    float anim_time = 0.0f;

    sogv_log_v("mesh count: %zu", mod->mesh_count);

    while(game.running) {
        while(SDL_PollEvent(&game.sdl_event)!=0) {
            if(game.sdl_event.type == SDL_QUIT) game.running = false;
            sogv_cam_handle_events(&cam, game.sdl_event);
        }
        sogv_base_loop_start(game);

        sogv_cam_movement(&cam, game.elapsed_ticks);

        mat4x4 view;
        vec3 cam_view;
        vec3_add(cam_view, cam.position, cam.front);
        mat4x4_look_at(view, cam.position, cam_view, cam.up);

        mat4x4 vp;
        mat4x4_mul(vp, proj, view);

        mat4x4 model, model_normal;
        mat4x4_identity(model);

        mat4x4_invert(model_normal, model);
        mat4x4_transpose(model_normal, model_normal);

        //mat4x4_translate_in_place(model, 0.0f, -1.5f, 0.0f);

        mat4x4 bones;
        mat4x4_identity(bones);

        glUseProgram(shader);
        char name[64];
        for(size_t i=0; i<MAX_BONES; ++i) {
            sprintf(name, "bones_mat[%zu]", i);
            sogv_gl_uniform_set_mat4x4(shader, name, bones);
        }

        mat4x4 test;
        mat4x4_identity(test);
        anim_time += game.elapsed_ticks*mod->anim_ticks;
        if(anim_time>=mod->anim_dur) anim_time -= mod->anim_dur;
        sogv_skel_animate(mod->root_node, anim_time, test, mod->bones, anim);
        sogv_gl_uniform_set_mat4x4_v(shader, mod->bone_count, "bones_mat[0]", anim[0]);

        sogv_gl_uniform_set_mat4x4(shader, "vp", vp);
        sogv_gl_uniform_set_mat4x4(shader, "model", model);
        sogv_gl_uniform_set_mat4x4(shader, "normal_mat", model_normal);
        sogv_model_render(mod);

        glUseProgram(shader2);
        mat4x4 model2, model_normal2;
        mat4x4_identity(model2);
        mat4x4_translate(model2, -2.0f, 0.0f, 0.0f);

        mat4x4_invert(model_normal2, model2);
        mat4x4_transpose(model_normal2, model_normal2);
        sogv_gl_uniform_set_mat4x4(shader2, "vp", vp);
        sogv_gl_uniform_set_mat4x4(shader2, "model", model2);
        sogv_gl_uniform_set_mat4x4(shader2, "normal_mat", model_normal2);
        sogv_model_render(mod2);

        sogv_base_loop_end(game);
    }

    sogv_model_free(mod);
    sogv_base_clean(&game);
    
    return EXIT_SUCCESS;
}
