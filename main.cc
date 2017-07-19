
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>


#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <cmath>
#include <functional>
#include <chrono>
#include <thread>


const std::vector<char> get_file_content(const std::string& file_name) {
    std::ifstream file(file_name, std::ios::binary);
    if( !file.good() ) {
        std::cout << "Could not open file!" << std::endl;
        perror("Error");

        std::exit(EXIT_FAILURE);
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);

    file.close();

    return buffer;
}


GLuint create_compiled_shader_from(
        GLenum shaderType,
        const std::string& file_name
    ) {
    std::cout << ">>> Shader '" << file_name << "':" << std::endl;

    GLuint id = glCreateShader(shaderType);

    if( !id ) {
        std::cout << "Could not create!" << std::endl;
        perror("Error");

        std::exit(EXIT_FAILURE);
    }

    const std::vector<char> buffer = get_file_content(file_name);
    const char* source = buffer.data();
    const int length = buffer.size();
    std::cout << "Source code:" << std::endl;
    std::cout << source << std::endl;

    glShaderSource(id, 1, &source, &length);

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
    if( compile_status == GL_FALSE ) {
        std::cout << "Could not compile!" << std::endl;
        perror("Error");

        std::exit(EXIT_FAILURE);
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

        std::exit(EXIT_FAILURE);
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

        std::exit(EXIT_FAILURE);
    }

    return id;
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
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

    GLint position_location = glGetAttribLocation(program_id, "position");
    GLint color_location = glGetAttribLocation(program_id, "color");


    size_t triangles = 128;
    Vertex circle[triangles + 2];
    glm::vec3 circle_starting_position(0.275, 0, 0.325);
    circle[0].position = glm::vec3(0, 0, 0) + circle_starting_position;
    float segment_angle = (2 * M_PI) / triangles;
    for( size_t i = 1; i < sizeof(circle) / sizeof(Vertex); i++ ) {
        float angle = (i - 1) * segment_angle;
        circle[i].position = glm::vec3(
                std::cos(angle),
                std::sin(angle),
                0
        ) * 0.125f + circle_starting_position;
    }

    circle[0].color = glm::u8vec4(255, 255, 255, 255);
    for( size_t i = 1; i < sizeof(circle) / sizeof(Vertex); i++ )
        circle[i].color = glm::u8vec4(255, 255, 0, 255);

    GLuint circle_vbo;
    glCreateBuffers(1, &circle_vbo);
    glNamedBufferStorage(circle_vbo, sizeof(circle), circle, GL_DYNAMIC_STORAGE_BIT);

    GLuint circle_vao;
    glCreateVertexArrays(1, &circle_vao);
    glBindVertexArray(circle_vao);
    glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    Vertex lines[6];
    lines[0].position = glm::vec3(2, 0, 0);
    lines[1].position = glm::vec3(-2, 0, 0);
    lines[2].position = glm::vec3(0, 2, 0);
    lines[3].position = glm::vec3(0, -2, 0);
    lines[4].position = glm::vec3(0, 0, 2);
    lines[5].position = glm::vec3(0, 0, -2);

    lines[0].color = glm::u8vec4(255, 0, 0, 255);
    lines[1].color = glm::u8vec4(0, 0, 255, 255);
    lines[2].color = glm::u8vec4(255, 0, 0, 255);
    lines[3].color = glm::u8vec4(0, 0, 255, 255);
    lines[4].color = glm::u8vec4(255, 0, 0, 255);
    lines[5].color = glm::u8vec4(0, 0, 255, 255);

    GLuint lines_vao, lines_vbo;
    glCreateBuffers(1, &lines_vbo);
    glNamedBufferStorage(lines_vbo, sizeof(lines), lines, 0);

    glCreateVertexArrays(1, &lines_vao);
    glBindVertexArray(lines_vao);
    glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    Vertex cube[8];
    cube[0].position = glm::vec3(-1,  1,  1);
    cube[1].position = glm::vec3(-1, -1,  1);
    cube[2].position = glm::vec3( 1,  1,  1);
    cube[3].position = glm::vec3( 1, -1,  1);
    cube[4].position = glm::vec3( 1,  1, -1);
    cube[5].position = glm::vec3( 1, -1, -1);
    cube[6].position = glm::vec3(-1,  1, -1);
    cube[7].position = glm::vec3(-1, -1, -1);

    glm::vec3 cube_starting_position(-0.275, 0, 0);
    for( size_t i = 0; i < sizeof(cube) / sizeof(Vertex); i++ )
        cube[i].position = cube[i].position * 0.125f + cube_starting_position;

    cube[0].color = glm::vec4(255,   0,   0, 255);
    cube[1].color = glm::vec4(  0, 255,   0, 255);
    cube[2].color = glm::vec4(  0,   0, 255, 255);
    cube[3].color = glm::vec4(255, 255, 255, 255);
    cube[4].color = glm::vec4(  0, 255,   0, 255);
    cube[5].color = glm::vec4(255,   0,   0, 255);
    cube[6].color = glm::vec4(255, 255, 255, 255);
    cube[7].color = glm::vec4(  0,   0, 255, 255);

    uint8_t cube_elements[17] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        0xFF,
        2, 4, 0, 6, 1, 7, 3, 5
    };

    GLuint cube_vao, cube_ebo, cube_vbo;
    glCreateBuffers(1, &cube_vbo);
    glNamedBufferStorage(cube_vbo, sizeof(cube), cube, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &cube_ebo);
    glNamedBufferStorage(cube_ebo, sizeof(cube_elements), cube_elements, 0);

    glCreateVertexArrays(1, &cube_vao);
    glBindVertexArray(cube_vao);
    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
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
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    glEnable(GL_DEPTH_TEST);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFF);


    glClearColor(0, 0, 0, 1);


    double spf = 1.0 / 60;
    while( !glfwWindowShouldClose(window) ) {
        double frame_start = glfwGetTime();

        {  // animating
            glm::mat3 rotate_z(
                     std::cos(spf), std::sin(spf), 0,
                    -std::sin(spf), std::cos(spf), 0,
                                 0,             0, 1
            );
            glm::mat3 rotate_y(
                     std::cos(spf),              0, -std::sin(spf),
                                 0,              1,              0,
                     std::sin(spf),              0,  std::cos(spf)
            );

            for( size_t i = 0; i < sizeof(circle) / sizeof(Vertex); i++ )
                circle[i].position = rotate_z * circle[i].position;

            glNamedBufferSubData(circle_vbo, 0, sizeof(circle), circle);


            for( size_t i = 0; i < sizeof(cube) / sizeof(Vertex); i++ )
                cube[i].position = rotate_y * cube[i].position;

            glNamedBufferSubData(cube_vbo, 0, sizeof(cube), cube);
        }

        {  // drawing
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(program_id);

            glBindVertexArray(circle_vao);
            glEnableVertexAttribArray(position_location);
            glEnableVertexAttribArray(color_location);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(circle) / sizeof(Vertex));
            glBindVertexArray(0);

            glBindVertexArray(lines_vao);
            glEnableVertexAttribArray(position_location);
            glEnableVertexAttribArray(color_location);
            glDrawArrays(GL_LINES, 0, sizeof(lines) / sizeof(Vertex));
            glBindVertexArray(0);

            glBindVertexArray(cube_vao);
            glEnableVertexAttribArray(position_location);
            glEnableVertexAttribArray(color_location);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);
            glDrawElements(GL_TRIANGLE_STRIP, sizeof(cube_elements), GL_UNSIGNED_BYTE, NULL);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            glUseProgram(0);

            glfwSwapBuffers(window);
        }

        {  // event handling
            glfwPollEvents();
        }

        double time_surplus = spf - (glfwGetTime() - frame_start);
        if( time_surplus > 0 )
            std::this_thread::sleep_for(
                    std::chrono::milliseconds((size_t) (time_surplus * 1000))
            );
    }

    return EXIT_SUCCESS;
}

