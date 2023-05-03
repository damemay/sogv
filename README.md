# sogv
SDL2/OpenGL/assimp base written in C that was supposed to be [vitaGL](https://github.com/Rinnegatamante/vitaGL) compatible *(and somewhat is)*.

**Project abandoned and not refactored after deciding to move onto learning Vulkan (hence archived).**

Do not think of it as anything usable, but the source code may come in handy for someone when learning OpenGL/Graphics programming, especially in C.

## the idea

- Hide all the boilerplate behind simple functions/macros and structs, while still leaving direct OpenGL functionality to the user.

    *Example: cut down code that initializes SDL2 + OpenGL, mesh + shader and a movable camera*

    ```c
    sogv_base game = sogv_base_create("newgl", WIDTH, HEIGHT,
            SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
            
    sogv_model* mod = sogv_model_create("../res/models/animation2/", "untitled.gltf");
    
    sogv_cam cam = sogv_cam_create(0.0f, 0.0f, 3.0f, 2.5f, 50.0f);
    
    GLuint shader = sogv_gl_shader_create("../res/shaders/normal_shader.vert",
            "../res/shaders/normal_shader.frag");
            
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

        glUseProgram(shader);
        sogv_gl_uniform_set_mat4x4(shader, "vp", vp);
        sogv_gl_uniform_set_mat4x4(shader, "model", model);
        sogv_gl_uniform_set_mat4x4(shader, "normal_mat", model_normal);
        sogv_model_render(mod);

        sogv_base_loop_end(game);
    }

    sogv_model_free(mod);
    sogv_base_clean(&game);
    ```

## what does it use

- [SDL2](https://github.com/libsdl-org/SDL) for windowing, simple key & controller handling for movement;
- [glad](https://github.com/Dav1dde/glad) for loading OpenGL 3.30 core;
- [assimp](https://github.com/assimp/assimp) loads in models and animations. Animation system was built based on [Anton's OpenGL book](https://antongerdelan.net/opengl/);
- [stb_image.h](https://github.com/nothings/stb) for decompressing images for OpenGL textures;
- [linmath.h](https://github.com/datenwolf/linmath.h) for most of the 3d mathematics;

### for building and using it on vita:
- [Northfear's fork of SDL2](https://github.com/Northfear/SDL/tree/vitagl)
- [vitaGL](https://github.com/Rinnegatamante/vitaGL)
