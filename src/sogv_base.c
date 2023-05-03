#include <SDL2/SDL.h>
#include <sogv.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

char* sogv_read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if(!file) perror("file"), exit(EXIT_FAILURE);

    fseek(file, 0L, SEEK_END);
    long size = ftell(file);
    fseek(file, 0L, SEEK_SET);

    char* code = malloc(size+1);
    if(!code)
        sogv_die("Could not allocate memory for file"),
        fclose(file), exit(EXIT_FAILURE);

    if(fread(code, size, 1, file) != 1)
        sogv_die("Could not read file"),
        fclose(file), free(code), exit(EXIT_FAILURE);

    fclose(file);
    code[size] = '\0';
    return code;
}

static void sdl_init(uint32_t flags) {
    if(SDL_Init(flags) < 0) sogv_die_v("Could not init SDL: %s", SDL_GetError());
    atexit(SDL_Quit);
}

static SDL_GameController** sdl_gamepads_init() {
    int joysticks;
    if((joysticks=SDL_NumJoysticks()) > 0) {
        SDL_GameController** gamepads = calloc(joysticks, sizeof(SDL_GameController*));
        for(int i=0; i<joysticks; i++)
            gamepads[i] = SDL_GameControllerOpen(i);
        return gamepads;
    } else return NULL;
}

static void sdl_gamepads_clean(SDL_GameController** gamepads) {
    for(int i=0; i<sizeof(gamepads); i++)
        SDL_GameControllerClose(gamepads[i]);
    free(gamepads);
}

static void sdl_create_window(SDL_Window** window, const char* title, const int width, const int height) {
    *window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if(!*window) sogv_die_v("Could not create SDL Window: %s", SDL_GetError());
}

static void sdl_clean(SDL_Window** window) {
    SDL_DestroyWindow(*window);
    *window = NULL;
}

static void sdl_gl_init() {
    #ifndef __vita__
        SDL_GL_LoadLibrary(NULL);
        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
        SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
        SDL_GL_CONTEXT_PROFILE_CORE);
    #else
        // vglInitExtended(0, 960, 544, 256 * 1024 * 1024, SCE_GXM_MULTISAMPLE_NONE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    #endif
}

static void sdl_gl_create_context(SDL_Window** window, SDL_GLContext* context, const int width, const int height) {
    *context = SDL_GL_CreateContext(*window);
    if(!*context) sogv_die_v("Could not create SDL GL Context: %s", SDL_GetError());

    SDL_GL_MakeCurrent(*window, *context);
    
    #ifndef __vita__
        if(!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
            exit(EXIT_FAILURE);
    #endif

    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    sogv_gl_check("creating context");
}

static void sdl_gl_clean(SDL_GLContext* context) {
    SDL_GL_DeleteContext(*context);
}

sogv_base sogv_base_create(const char* title, int w, int h, const uint32_t sdl_flags) {
    sogv_base new;
    new.window = NULL;
    new.context = NULL;
    new.running = true;

    sogv_log("Initializing SDL...");
    sdl_init(sdl_flags);
    sogv_log("Initializing GL...");
    sdl_gl_init();
    sogv_log("Creating window...");
    sdl_create_window(&new.window, title, w, h);
    sogv_log("Creating GL Context inside SDL...");
    sdl_gl_create_context(&new.window, &new.context, w, h);
    sogv_log("Initializing SDL Gamepads...");
    new.gamepads = sdl_gamepads_init();

    new.last_tick = SDL_GetTicks64();

    return new;
}

void sogv_base_clean(sogv_base* b) {
    if(b->gamepads) sdl_gamepads_clean(b->gamepads);
    sdl_gl_clean(&b->context);
    sdl_clean(&b->window);
}

static char* gl_parse_err(const GLenum code) {
    char* out;
    switch(code) {
        sogv_parse_enum(GL_INVALID_ENUM);
        sogv_parse_enum(GL_INVALID_VALUE);
        sogv_parse_enum(GL_INVALID_OPERATION);
        sogv_parse_enum(GL_OUT_OF_MEMORY);
        #ifndef __vita__
        sogv_parse_enum(GL_INVALID_FRAMEBUFFER_OPERATION);
        #endif
    }
    return out;
}

void sogv_gl_check(const char* msg) {
    GLenum code;
    while((code = glGetError()) != GL_NO_ERROR)
        printf("[GL]\t%s when %s.\n", gl_parse_err(code), msg);
}

static GLuint gl_shader_compile(const GLenum type, const char* code) {
    GLuint shader = glCreateShader(type);
    int success;
    char gl_log[1024];

    glShaderSource(shader, 1, &code, NULL);
    glCompileShader(shader);
    sogv_gl_check("compiling shader");

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success)
        glGetShaderInfoLog(shader, 512, NULL, gl_log),
        sogv_die_v("Compiling shader failed: %s", gl_log);

    sogv_gl_check("shader should have compiled");
    sogv_gl_check("activating texture");
    return shader;
}

static void gl_shader_link(GLuint* shader, const GLuint vertex, const GLuint fragment) {
    int success;
    char gl_log[512];

    *shader = glCreateProgram();
    sogv_gl_check("creating shader program");

    glAttachShader(*shader, vertex);
    sogv_gl_check("attaching vertex shader");
    glAttachShader(*shader, fragment);
    sogv_gl_check("attaching fragment shader");

    glLinkProgram(*shader);
    sogv_gl_check("linking shader program");

    glGetProgramiv(*shader, GL_LINK_STATUS, &success);
    if(!success)
        glGetProgramInfoLog(*shader, 512, NULL, gl_log),
        sogv_die_v("Linking shader program failed: %s", gl_log);

    sogv_gl_check("shader program should have linked");
}

GLuint sogv_gl_shader_create(const char* vertex_path, const char* fragment_path) {
    GLuint new;

    char* vertex_code = sogv_read_file(vertex_path);
    GLuint vertex_shader = gl_shader_compile(GL_VERTEX_SHADER, vertex_code);
    free(vertex_code);

    char* fragment_code = sogv_read_file(fragment_path);
    GLuint fragment_shader = gl_shader_compile(GL_FRAGMENT_SHADER, fragment_code);
    free(fragment_code);

    gl_shader_link(&new, vertex_shader, fragment_shader);

    glDeleteShader(vertex_shader);
    sogv_gl_check("deleting vertex shader");

    glDeleteShader(fragment_shader);
    sogv_gl_check("deleting fragment shader");

    return new;
}

GLuint sogv_gl_stb_texture_create(const char* path) {
    GLuint id;
    glGenTextures(1, &id);

    int w, h, n;
    unsigned char* data = stbi_load(path, &w, &h, &n, 0);
    if(data) {
        sogv_log_v("STBI loaded %s! Binding into GL..", path);
        GLenum f;
        switch(n) {
            case 1: f = GL_RED; break;
            case 3: f = GL_RGB; break;
            case 4: f = GL_RGBA; break;
            default: sogv_die_v("STBI stumbled upon strange image format when loading: %s", path); break;
        }
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, f, w, h, 0, f, GL_UNSIGNED_BYTE, data);
        sogv_gl_check("tex image2d");
        glGenerateMipmap(GL_TEXTURE_2D);
        sogv_gl_tex_parameterize(GL_TEXTURE_2D, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
        sogv_gl_check("parametearierhzeir");
        stbi_image_free(data);
    } else {
        stbi_image_free(data);
        sogv_die_v("STBI could not load image: %s", path);
    }

    return id;
}
