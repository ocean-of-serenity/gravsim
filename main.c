
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <GL/glew.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <iso646.h>


const char* load_shader_source(const char* file_name) {
    FILE* file = fopen(file_name, "r");

    if( file == NULL ) {
        printf("Could not open file '%s':\n", file_name);
        perror("Error");

        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = calloc(size + 1, sizeof(char));
    fread(buffer, sizeof(char), size, file);

    fclose(file);

    return buffer;
}


GLuint create_compiled_shader_from(GLenum shaderType, const char* file_name) {
    GLuint id = glCreateShader(shaderType);

    if( !id ) {
        printf("Could not create shader\n");
        perror("Error");

        exit(1);
    }

    const char* source = load_shader_source(file_name);
    glShaderSource(id, 1, &source, NULL);

    glCompileShader(id);

    GLint compile_status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);

    if( compile_status == GL_FALSE ) {
        GLint info_log_length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);

        char info_log[info_log_length];
        GLsizei temp;
        glGetShaderInfoLog(id, info_log_length, &temp, info_log);

        printf(
                "Could not compile shader from '%s':\n%s\nsource code:\n%s\n",
                file_name,
                info_log,
                source
        );
        perror("Error");

        free((void*) source);

        exit(1);
    }

    free((void*) source);

    return id;
}


GLuint create_linked_program_with(
        GLuint vertex_shader_id,
        GLuint fragment_shader_id
    ) {
    GLuint id = glCreateProgram();
    
    if( !id ) {
        printf("Could not create program\n");
        perror("Error");

        exit(1);
    }

    glAttachShader(id, vertex_shader_id);
    glAttachShader(id, fragment_shader_id);

    glLinkProgram(id);

    glDetachShader(id, vertex_shader_id);
    glDetachShader(id, fragment_shader_id);

    GLint validate_status;
    glGetProgramiv(id, GL_VALIDATE_STATUS, &validate_status);

    GLint link_status;
    glGetProgramiv(id, GL_LINK_STATUS, &link_status);

    if( validate_status == GL_FALSE or link_status == GL_FALSE ) {
        GLint info_log_length;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_length);

        char info_log[info_log_length];
        GLsizei temp;
        glGetProgramInfoLog(id, info_log_length, &temp, info_log);

        printf("Could not link program:\n%s\n", info_log);
        perror("Error");

        glDeleteProgram(id);

        exit(1);
    }

    return id;
}


const uint8_t* load_png(const char* file_name) {
    FILE* file = fopen(file_name, "rb");

    if( file == NULL ) {
        char* fmt_msg = "Could not open file '%s':";
        char msg[strlen(file_name) + strlen(fmt_msg) + 2];
        sprintf(msg, fmt_msg, file_name);
        printf("Could not open file '%s':\n", file_name);
        perror("Error");

        exit(1);
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

void set_vertex_color(
        Vertex* vertex,
        uint8_t r,
        uint8_t g,
        uint8_t b,
        uint8_t a
) {
    vertex->color.r = r;
    vertex->color.g = g;
    vertex->color.b = b;
    vertex->color.a = a;
}


int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
            "opengl_test",
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

    GLuint vertex_shader_id = create_compiled_shader_from(
            GL_VERTEX_SHADER,
            "vertex_shader.glsl"
    );
    GLuint fragment_shader_id = create_compiled_shader_from(
            GL_FRAGMENT_SHADER,
            "fragment_shader.glsl"
    );

    GLuint program_id = create_linked_program_with(
            vertex_shader_id,
            fragment_shader_id
    ); 

    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);


    size_t polygons = 8;
    size_t vertices = polygons * 3;
    Vertex buffer[vertices];

    size_t i = 0;

    set_vertex_position(buffer + i++,  -0.25, 0.5, 0.0);
    set_vertex_position(buffer + i++,  0.25, 0.5, 0.0);
    set_vertex_position(buffer + i++,  0.0, 0.0, 0.0);

    set_vertex_position(buffer + i++,  0.25, 0.5, 0.0);
    set_vertex_position(buffer + i++,  0.5, 0.25, 0.0);
    set_vertex_position(buffer + i++,  0.0, 0.0, 0.0);

    set_vertex_position(buffer + i++,  0.5, 0.25, 0.0);
    set_vertex_position(buffer + i++,  0.5, -0.25, 0.0);
    set_vertex_position(buffer + i++,  0.0, 0.0, 0.0);

    set_vertex_position(buffer + i++,  0.5, -0.25, 0.0);
    set_vertex_position(buffer + i++,  0.25, -0.5, 0.0);
    set_vertex_position(buffer + i++,  0.0, 0.0, 0.0);

    set_vertex_position(buffer + i++,  0.25, -0.5, 0.0);
    set_vertex_position(buffer + i++,  -0.25, -0.5, 0.0);
    set_vertex_position(buffer + i++,  0.0, 0.0, 0.0);

    set_vertex_position(buffer + i++,  -0.25, -0.5, 0.0);
    set_vertex_position(buffer + i++,  -0.5, -0.25, 0.0);
    set_vertex_position(buffer + i++,  0.0, 0.0, 0.0);

    set_vertex_position(buffer + i++,  -0.5, -0.25, 0.0);
    set_vertex_position(buffer + i++,  -0.5, 0.25, 0.0);
    set_vertex_position(buffer + i++,  0.0, 0.0, 0.0);

    set_vertex_position(buffer + i++,  -0.5, 0.25, 0.0);
    set_vertex_position(buffer + i++,  -0.25, 0.5, 0.0);
    set_vertex_position(buffer + i++,  0.0, 0.0, 0.0);

    for( i = 0; i < vertices; i++ ) {
        if( i % 3 == 0 or i % 3 == 1 )
            set_vertex_color(buffer + i,  255, 255, 0, 255);
        else
            set_vertex_color(buffer + i,  255, 255, 255, 255);
    }


    GLuint buffer_id;
    glGenBuffers(1, &buffer_id);

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

    float counter = 0;
    while( true ) {
        {  // event handling
            SDL_Event event;
            while( SDL_PollEvent(&event) ) {
                if( event.type == SDL_QUIT ) {
                    glDeleteProgram(program_id);
                    glDeleteBuffers(1, &buffer_id);
                    SDL_GL_DeleteContext(context);
                    SDL_DestroyWindow(window);
                    SDL_Quit();
                    
                    exit(0);
                }
            }
        }

        {  // drawing
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClearDepth(1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(program_id);

            glUniform1f(glGetUniformLocation(program_id, "time"), counter);

            glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);

            glVertexAttribPointer(
                    0,
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    sizeof(Vertex),
                    (void*) offsetof(Vertex, position)
            );
            glVertexAttribPointer(
                    1,
                    4,
                    GL_UNSIGNED_BYTE,
                    GL_TRUE,
                    sizeof(Vertex),
                    (void*) offsetof(Vertex, color)
            );

            glDrawArrays(GL_TRIANGLES, 0, sizeof(buffer) / sizeof(Vertex));
            
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glUseProgram(0);

            SDL_GL_SwapWindow(window);
        }

        counter += 0.01;
    }

    return 1;
}



