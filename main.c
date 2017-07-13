
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/glew.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


void debug(const char* debug_text) {
    printf("%s\n", debug_text);
}


const char* load_shader(const char* file_name) {
    FILE* file = fopen(file_name, "r");

    if( file == NULL ) {
        char* fmt_msg = "Could not open file '%s':";
        char msg[strlen(file_name) + strlen(fmt_msg) + 2];
        sprintf(msg, fmt_msg, file_name);
        perror(msg);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = calloc(size + 1, sizeof(char));
    fread(buffer, sizeof(char), size, file);

    fclose(file);

    return buffer;
}


const uint8_t* load_png(const char* file_name) {
    FILE* file = fopen(file_name, "rb");

    if( file == NULL ) {
        char* fmt_msg = "Could not open file '%s':";
        char msg[strlen(file_name) + strlen(fmt_msg) + 2];
        sprintf(msg, fmt_msg, file_name);
        perror(msg);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t* buffer = calloc(size + 1, sizeof(uint8_t));
    fread(buffer, sizeof(uint8_t), size, file);

    fclose(file);

    return buffer;
}


typedef struct Position {
    float x;
    float y;
    float z;
} Position;

typedef struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} Color;

typedef struct Vertex {
    Position position;
    Color color;    
} Vertex;

void set_vertex_position(Vertex* vertex, float x, float y, float z) {
    vertex->position.x = x;
    vertex->position.y = y;
    vertex->position.z = z;
}

void set_vertex_color(Vertex* vertex, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    vertex->color.r = r;
    vertex->color.g = g;
    vertex->color.b = b;
    vertex->color.a = a;
}


int main(int argc, char** argv) {
    debug("initializing ...");
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
            "gl_test",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            1280,
            720,
            SDL_WINDOW_OPENGL
    );

    SDL_GLContext context = SDL_GL_CreateContext(window);

    glewInit();

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    GLint shader_compile_status;
    GLint shader_info_log_length;
    GLsizei temp;

    debug("vertex");
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    const char* vertex_shader = load_shader("vertex_shader.glsl");
    glShaderSource(vertex_shader_id, 1, &vertex_shader, NULL);
    glCompileShader(vertex_shader_id);
    glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &shader_compile_status);
    glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &shader_info_log_length);
    char vertex_info_log[shader_info_log_length];
    glGetShaderInfoLog(vertex_shader_id, shader_info_log_length, &temp, vertex_info_log);
    debug(vertex_info_log);
    debug(vertex_shader);
    if(shader_compile_status == GL_FALSE)
        return 0;
    free((void*) vertex_shader);

    debug("fragment");
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragment_shader = load_shader("fragment_shader.glsl");
    glShaderSource(fragment_shader_id, 1, &fragment_shader, NULL);
    glCompileShader(fragment_shader_id);
    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &shader_compile_status);
    glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &shader_info_log_length);
    char fragment_info_log[shader_info_log_length];
    glGetShaderInfoLog(fragment_shader_id, shader_info_log_length, &temp, fragment_info_log);
    debug(fragment_info_log);
    debug(fragment_shader);
    if(shader_compile_status == GL_FALSE)
        return 0;
    free((void*) fragment_shader);

    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);
    glLinkProgram(program_id);
    glDetachShader(program_id, vertex_shader_id);
    glDetachShader(program_id, fragment_shader_id); 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    GLuint buffer_id;
    glGenBuffers(1, &buffer_id);

    Vertex buffer[6];
    set_vertex_position(buffer + 0, -0.5, 0.5, 0.0);
    set_vertex_color(buffer + 0, 255, 255, 0.0, 255);
    set_vertex_position(buffer + 1, -0.5, -0.5, 0.0);
    set_vertex_color(buffer + 1, 0.0, 0.0, 255, 255);
    set_vertex_position(buffer + 2, 0.5, -0.5, 0.0);
    set_vertex_color(buffer + 2, 255, 255, 0.0, 255);
    set_vertex_position(buffer + 3, 0.5, -0.5, 0.0);
    set_vertex_color(buffer + 3, 255, 255, 0.0, 255);
    set_vertex_position(buffer + 4, 0.5, 0.5, 0.0);
    set_vertex_color(buffer + 4, 0.0, 0.0, 255, 255);
    set_vertex_position(buffer + 5, -0.5, 0.5, 0.0);
    set_vertex_color(buffer + 5, 255, 255, 0.0, 255);

    glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
    glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(buffer),
            buffer,
            GL_STATIC_DRAW
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindAttribLocation(program_id, 0, "position");
    glBindAttribLocation(program_id, 1, "color");

    debug("entering main loop");
    float blub = 0;
    while( true ) {
        {  // event handling
            SDL_Event event;
            while( SDL_PollEvent(&event) ) {
                switch( event.type ) {
                    case SDL_QUIT:
                        debug("quit event received");
                        glDeleteBuffers(1, &buffer_id);
                        SDL_GL_DeleteContext(context);
                        SDL_DestroyWindow(window);
                        SDL_Quit();
                        return 0;
                }
            }
        }

        {  // drawing
            glClearColor(0.0, 0.0, 1.0, 1.0);
            glClearDepth(1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(program_id);

            glUniform1f(glGetUniformLocation(program_id, "time"), blub);

            glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(
                    0,
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    sizeof(Vertex),
                    (void*) 0
            );
            glVertexAttribPointer(
                    1,
                    4,
                    GL_UNSIGNED_BYTE,
                    GL_TRUE,
                    sizeof(Vertex),
                    (void*) 12
            );

            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glUseProgram(0);

            SDL_GL_SwapWindow(window);
        }

        blub += 0.01;
    }
}



