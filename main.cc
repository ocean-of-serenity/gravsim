
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <cmath>
#include <functional>


const std::vector<char> load_shader_source(const std::string& file_name) {
    std::ifstream file(file_name);
    if( !file.good() ) {
        std::cout << "Could not open file!" << std::endl;
        perror("Error");

        exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size + 1);
    file.read(buffer.data(), size);

    file.close();

    return buffer;
}


GLuint create_compiled_shader_from(GLenum shaderType, const std::string& file_name) {
    std::cout << ">>> Shader '" << file_name << "':" << std::endl;

    GLuint id = glCreateShader(shaderType);

    if( !id ) {
        std::cout << "Could not create!" << std::endl;
        perror("Error");

        std::exit(EXIT_FAILURE);
    }

    const char* source = load_shader_source(file_name).data();
    glShaderSource(id, 1, &source, NULL);

    glCompileShader(id);

    GLint compile_status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);

    GLint info_log_length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);

    char info_log[info_log_length];
    GLsizei temp;
    glGetShaderInfoLog(id, info_log_length, &temp, info_log);

    std::cout << "Info log:" << std::endl;
    std::cout << info_log << std::endl;
    std::cout << "Source code:" << std::endl;
    std::cout << source << std::endl;

    if( compile_status == GL_FALSE ) {
        std::cout << "Could not compile!" << std::endl;
        perror("Error");

        exit(EXIT_FAILURE);
    }

    return id;
}


GLuint create_linked_program_with(
        GLuint vertex_shader_id,
        GLuint fragment_shader_id
    ) {
    std::cout << ">>> Program:" << std::endl;
    GLuint id = glCreateProgram();
    
    if( !id ) {
        std::cout << "Could not create!" << std::endl;
        perror("Error");

        exit(EXIT_FAILURE);
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

    GLint info_log_length;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &info_log_length);

    char info_log[info_log_length];
    GLsizei temp;
    glGetProgramInfoLog(id, info_log_length, &temp, info_log);

    std::cout << "Info log:" << std::endl;
    std::cout << info_log << std::endl;

    if( validate_status == GL_FALSE or link_status == GL_FALSE ) {
        std::cout << "Could not link!" << std::endl;
        perror("Error");

        glDeleteProgram(id);

        exit(EXIT_FAILURE);
    }

    return id;
}


const std::vector<uint8_t> load_png(const std::string& file_name) {
    std::ifstream file(file_name, std::ios::binary);

    if( !file.good() ) {
        std::cout << "Could not open file " << file_name << std::endl;
        perror("Error");

        exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    file.read((char*) buffer.data(), size);

    file.close();

    return buffer;
}


class Deferer {
public:
    explicit Deferer() : callbacks() {}
    Deferer(const Deferer&) = delete;
    Deferer& operator=(const Deferer&) = delete;

    ~Deferer() {
        while( !this->callbacks.empty() ) {
            std::function<void()> callback = this->callbacks.back();
            this->callbacks.pop_back();
            callback();
        }
    }

    void operator()(std::function<void()> callback) {
        this->callbacks.push_back(callback);
    }

private:
    std::deque<std::function<void()>> callbacks;
};


typedef struct Vertex {
    glm::vec3 position;
    glm::u8vec4 color;    
} Vertex;


int main(int argc, char** argv) {
    Deferer defer;

    glfwSetErrorCallback([](int error, const char* description) -> void {
            std::cout << "GLFW Error: " << error << std::endl;
            std::cout << description << std::endl;
    });

    if( !glfwInit() ) {
        std::cout << "Failed to initialize glfw" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    defer([]() -> void { glfwTerminate(); });

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    
    GLFWwindow* window = glfwCreateWindow(720, 720, "opengl_test", NULL, NULL);
    if( !window ) {
        std::cout << "Failed to create window" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    defer([window]() -> void { glfwDestroyWindow(window); });

    glfwMakeContextCurrent(window);

    if( glewInit() != GLEW_OK ) {
        std::cout << "Failed to initialize glew" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    glfwSwapInterval(1);


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


    size_t triangles = 128;
    Vertex buffer[triangles + 2];  // + 2 => origin point and end/start connection

    glm::vec3 starting_position(0.875, 0, 0);
    buffer[0].position = glm::vec3(0, 0, 0) + starting_position;
    float segment_angle = (2 * M_PI) / triangles;
    for( size_t i = 1; i < sizeof(buffer) / sizeof(Vertex); i++ ) {
        float angle = (i - 1) * segment_angle;
        buffer[i].position = glm::vec3(cos(angle), sin(angle), 0) * 0.125f + starting_position;
    }

    buffer[0].color = glm::u8vec4(255, 255, 255, 255);
    for( size_t i = 1; i < sizeof(buffer) / sizeof(Vertex); i++ )
        buffer[i].color = glm::u8vec4(255, 255, 0, 255);


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


    float counter = 0;
    while( !glfwWindowShouldClose(window) ) {
        {  // drawing
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClearDepth(1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(program_id);

            glBindBuffer(GL_ARRAY_BUFFER, buffer_id);

            GLint time_location = glGetUniformLocation(program_id, "time");
            glUniform1f(time_location, counter);

            GLint position_location = glGetAttribLocation(program_id, "position");
            GLint color_location = glGetAttribLocation(program_id, "color");

            glEnableVertexAttribArray(position_location);
            glEnableVertexAttribArray(color_location);

            glVertexAttribPointer(
                    position_location,
                    3,
                    GL_FLOAT,
                    GL_FALSE,
                    sizeof(Vertex),
                    (void*) offsetof(Vertex, position)
            );
            glVertexAttribPointer(
                    color_location,
                    4,
                    GL_UNSIGNED_BYTE,
                    GL_TRUE,
                    sizeof(Vertex),
                    (void*) offsetof(Vertex, color)
            );

            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(buffer) / sizeof(Vertex));
            
            glDisableVertexAttribArray(color_location);
            glDisableVertexAttribArray(position_location);

            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glUseProgram(0);

            glfwSwapBuffers(window);
        }

        {  // event handling
            glfwPollEvents();
        }

        counter += 0.01;
    }

    return EXIT_SUCCESS;
}

