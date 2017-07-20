
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    std::cout.write(source, length);
    std::cout << std::endl;

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
    std::cout.write(info_log, info_log_length);
    std::cout << std::endl;
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
    GLint projection_location = glGetUniformLocation(program_id, "projection");
    GLint view_location = glGetUniformLocation(program_id, "view");
    GLint model_location = glGetUniformLocation(program_id, "model");


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
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(
            position_location,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(color_location);
    glVertexAttribPointer(
            color_location,
            4,
            GL_UNSIGNED_BYTE,
            GL_TRUE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, color)
    );

    glBindVertexArray(0);


    size_t triangles = 128;
    Vertex circle[triangles + 2];
    circle[0].position = glm::vec3(0, 0, 0);
    float segment_angle = (2 * M_PI) / triangles;
    for( size_t i = 1; i < sizeof(circle) / sizeof(Vertex); i++ ) {
        float angle = (i - 1) * segment_angle;
        circle[i].position = glm::vec3(
                std::cos(angle),
                std::sin(angle),
                0
        );
    }

    circle[0].color = glm::u8vec4(255, 255, 255, 255);
    for( size_t i = 1; i < sizeof(circle) / sizeof(Vertex); i++ )
        circle[i].color = glm::u8vec4(255, 255, 0, 255);

    GLuint circle_vao, circle_vbo;
    glCreateBuffers(1, &circle_vbo);
    glNamedBufferStorage(circle_vbo, sizeof(circle), circle, 0);

    glCreateVertexArrays(1, &circle_vao);
    glBindVertexArray(circle_vao);

    glBindBuffer(GL_ARRAY_BUFFER, circle_vbo);
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(
            position_location,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(color_location);
    glVertexAttribPointer(
            color_location,
            4,
            GL_UNSIGNED_BYTE,
            GL_TRUE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, color)
    );

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

    cube[0].color = glm::u8vec4(255,   0,   0, 255);
    cube[1].color = glm::u8vec4(  0, 255,   0, 255);
    cube[2].color = glm::u8vec4(  0,   0, 255, 255);
    cube[3].color = glm::u8vec4(255, 255, 255, 255);
    cube[4].color = glm::u8vec4(  0, 255,   0, 255);
    cube[5].color = glm::u8vec4(255,   0,   0, 255);
    cube[6].color = glm::u8vec4(255, 255, 255, 255);
    cube[7].color = glm::u8vec4(  0,   0, 255, 255);

    uint8_t cube_elements[17] = {
        0, 1, 2, 3, 4, 5, 6, 7,
        0xFF,
        2, 4, 0, 6, 1, 7, 3, 5
    };

    GLuint cube_vao, cube_ebo, cube_vbo;
    glCreateBuffers(1, &cube_vbo);
    glNamedBufferStorage(cube_vbo, sizeof(cube), cube, 0);

    glCreateBuffers(1, &cube_ebo);
    glNamedBufferStorage(cube_ebo, sizeof(cube_elements), cube_elements, 0);

    glCreateVertexArrays(1, &cube_vao);
    glBindVertexArray(cube_vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);

    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(
            position_location,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(color_location);
    glVertexAttribPointer(
            color_location,
            4,
            GL_UNSIGNED_BYTE,
            GL_TRUE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, color)
    );

    glBindVertexArray(0);


    Vertex octahedron[6];
    octahedron[0].position = glm::vec3( 0,  1,  0);
    octahedron[1].position = glm::vec3(-1,  0,  0);
    octahedron[2].position = glm::vec3( 0,  0,  1);
    octahedron[3].position = glm::vec3( 1,  0,  0);
    octahedron[4].position = glm::vec3( 0,  0, -1);
    octahedron[5].position = glm::vec3( 0, -1,  0);

    octahedron[0].color = glm::u8vec4(255,   0,   0, 255);
    octahedron[1].color = glm::u8vec4(  0, 255,   0, 255);
    octahedron[2].color = glm::u8vec4(  0,   0, 255, 255);
    octahedron[3].color = glm::u8vec4(255,   0, 255, 255);
    octahedron[4].color = glm::u8vec4(255, 255,   0, 255);
    octahedron[5].color = glm::u8vec4(  0, 255, 255, 255);

    uint8_t octahedron_elements[13] = {
        0, 1, 2, 3, 4, 1,
        0xFF,
        5, 1, 2, 3, 4, 1
    };

    GLuint octahedron_vao, octahedron_ebo, octahedron_vbo;
    glCreateBuffers(1, &octahedron_vbo);
    glNamedBufferStorage(octahedron_vbo, sizeof(octahedron), octahedron, 0);

    glCreateBuffers(1, &octahedron_ebo);
    glNamedBufferStorage(octahedron_ebo, sizeof(octahedron_elements), octahedron_elements, 0);

    glCreateVertexArrays(1, &octahedron_vao);
    glBindVertexArray(octahedron_vao);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, octahedron_ebo);

    glBindBuffer(GL_ARRAY_BUFFER, octahedron_vbo);
    glEnableVertexAttribArray(position_location);
    glVertexAttribPointer(
            position_location,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(color_location);
    glVertexAttribPointer(
            color_location,
            4,
            GL_UNSIGNED_BYTE,
            GL_TRUE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, color)
    );

    glBindVertexArray(0);


    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 10.0f);

    glm::mat4 view = glm::lookAt(
            glm::vec3(1, 1, 2),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
    );
    
    glm::mat4 lines_model(1);

    glm::mat4 circle_model = (
        glm::translate(
                glm::mat4(1),
                glm::vec3(0.2, 0, 0.2)
        ) *
        glm::scale(
                glm::mat4(1),
                glm::vec3(0.125, 0.125, 0.125)
        ) *
        glm::rotate(
                glm::mat4(1),
                glm::radians(45.0f),
                glm::vec3(0, 1, 0)
        )
    );

    glm::mat4 cube_model = (
            glm::translate(
                glm::mat4(1),
                glm::vec3(0.25, 0, 0)
        ) *
        glm::scale(
                glm::mat4(1),
                glm::vec3(0.125, 0.125, 0.125)
        ) *
        glm::rotate(
                glm::mat4(1),
                glm::radians(0.0f),
                glm::vec3(1, 0, 0)
        )
    );

    glm::mat4 octahedron_model = (
        glm::translate(
                glm::mat4(1),
                glm::vec3(0.5, 0, -0.25)
        ) *
        glm::scale(
                glm::mat4(1),
                glm::vec3(0.125, 0.125, 0.125)
        ) *
        glm::rotate(
                glm::mat4(1),
                glm::radians(0.0f),
                glm::vec3(1, 0, 0)
        )
    ); 


    glEnable(GL_DEPTH_TEST);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFF);

    glClearColor(0, 0, 0, 1);


    double spf = 1.0 / 60;
    while( !glfwWindowShouldClose(window) ) {
        double frame_start = glfwGetTime();

        {  // animating
            circle_model = glm::rotate(
                    glm::mat4(1),
                    glm::radians((float) (360 * spf / 4)),
                    glm::vec3(0, 0, 1)
            ) * circle_model;

            cube_model = glm::rotate(
                    glm::mat4(1),
                    glm::radians((float) (360 * spf / 4)),
                    glm::vec3(0, 1, 0)
            ) * cube_model;
            octahedron_model = glm::rotate(
                    glm::mat4(1),
                    glm::radians((float) (360 * spf / 4)),
                    glm::vec3(1, 0, 0)
            ) * octahedron_model;
        }

        {  // drawing
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(program_id);

            glUniformMatrix4fv(projection_location, 1, GL_FALSE, glm::value_ptr(projection));
            glUniformMatrix4fv(view_location, 1, GL_FALSE, glm::value_ptr(view));

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(lines_model));
            glBindVertexArray(lines_vao);
            glDrawArrays(GL_LINES, 0, sizeof(lines) / sizeof(Vertex));
            glBindVertexArray(0);

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(circle_model));
            glBindVertexArray(circle_vao);
            glDrawArrays(GL_TRIANGLE_FAN, 0, sizeof(circle) / sizeof(Vertex));
            glBindVertexArray(0);

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(cube_model));
            glBindVertexArray(cube_vao);
            glDrawElements(GL_TRIANGLE_STRIP, sizeof(cube_elements), GL_UNSIGNED_BYTE, NULL);
            glBindVertexArray(0);

            glUniformMatrix4fv(model_location, 1, GL_FALSE, glm::value_ptr(octahedron_model));
            glBindVertexArray(octahedron_vao);
            glDrawElements(GL_TRIANGLE_FAN, sizeof(octahedron_elements), GL_UNSIGNED_BYTE, NULL);
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

