
#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <unordered_map>
#include <cmath>
#include <functional>
#include <chrono>
#include <thread>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "base.h"
#include "sphere.h"


const std::vector<char> get_file_content(const std::string& file_name) {
    std::ifstream file(file_name, std::ios::binary);
    if( !file.good() ) {
        std::cout << "#>> Could not open file!" << std::endl;
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
    std::cout << "##>>> Shader '" << file_name << "':" << std::endl;

    GLuint id = glCreateShader(shaderType);

    if( !id ) {
        std::cout << "#>> Could not create!" << std::endl;

        std::exit(EXIT_FAILURE);
    }

    const std::vector<char> buffer = get_file_content(file_name);
    const char* source = buffer.data();
    const int length = buffer.size();
    glShaderSource(id, 1, &source, &length);

    glCompileShader(id);

    GLint compile_status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compile_status);

    GLint info_log_length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);

    char info_log[info_log_length];
    GLsizei temp;
    glGetShaderInfoLog(id, info_log_length, &temp, info_log);

    std::cout << "#>> Info log:" << std::endl;
    std::cout.write(info_log, info_log_length);
    std::cout << std::endl;
    if( compile_status == GL_FALSE ) {
        std::cout << "#>> Source code:" << std::endl;
        std::cout.write(source, length);
        std::cout << std::endl;
        std::cout << "#>> Could not compile!" << std::endl;

        std::exit(EXIT_FAILURE);
    }

    return id;
}


GLuint create_linked_program_with(
        GLuint vertex_shader_id,
        GLuint fragment_shader_id
    ) {
    std::cout << "##>>> Program:" << std::endl;
    GLuint id = glCreateProgram();
    
    if( !id ) {
        std::cout << "#>> Could not create!" << std::endl;

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

    std::cout << "#>> Info log:" << std::endl;
    std::cout.write(info_log, info_log_length);
    std::cout << std::endl;

    if( validate_status == GL_FALSE or link_status == GL_FALSE ) {
        std::cout << "#>> Could not link!" << std::endl;

        glDeleteProgram(id);

        std::exit(EXIT_FAILURE);
    }

    return id;
}


class Deferer {
private:
    std::deque<std::function<void()>> callbacks;
    bool done;

public:
    Deferer(const Deferer&) = delete;
    Deferer(const Deferer&&) = delete;
    Deferer& operator=(const Deferer&) = delete;
    Deferer& operator=(const Deferer&&) = delete;

    explicit Deferer()
        :   callbacks(),
            done(false)
    {}

    ~Deferer() {
        if( !this->done ) {
            while( !this->callbacks.empty() ) {
                std::function<void()> callback = this->callbacks.back();
                this->callbacks.pop_back();
                callback();
            }
        }
    }

    void operator()(std::function<void()> callback) {
        this->callbacks.push_back(callback);
    }

    void unfold() {
        if( !this->done ) {
            while( !this->callbacks.empty() ) {
                std::function<void()> callback = this->callbacks.back();
                this->callbacks.pop_back();
                callback();
            }

            this->done = true;
        }
    }
};


int main(const int argc, const char** const argv) {
    Deferer defer;

    glfwSetErrorCallback([](int error, const char* description) -> void {
            std::cout << "#>> GLFW Error: " << error << std::endl;
            std::cout << description << std::endl;
    });

    if( !glfwInit() ) {
        std::cout << "##>>> Failed to initialize glfw" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    defer([]() -> void { glfwTerminate(); });

    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    uint32_t width = 1280;
    uint32_t height = 720;
    GLFWwindow* window = glfwCreateWindow(
            width,
            height,
            "OpenGL Test",
            NULL,
            NULL
    );
    if( !window ) {
        std::cout << "##>>> Failed to create window" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    defer([window]() -> void { glfwDestroyWindow(window); });

    glfwMakeContextCurrent(window);

    if( glewInit() != GLEW_OK ) {
        std::cout << "##>>> Failed to initialize glew" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "#>> OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    glfwSwapInterval(1);


    GLuint sp_id;
    GLuint mp_id;
    {
        GLuint svs_id = create_compiled_shader_from(
                GL_VERTEX_SHADER,
                "static_vertex_shader.glsl"
        );
        GLuint mvs_id = create_compiled_shader_from(
                GL_VERTEX_SHADER,
                "moving_vertex_shader.glsl"
        );
        GLuint fs_id = create_compiled_shader_from(
                GL_FRAGMENT_SHADER,
                "fragment_shader.glsl"
        );
        
        sp_id = create_linked_program_with(svs_id, fs_id);
        mp_id = create_linked_program_with(mvs_id,fs_id); 
        
        glDeleteShader(svs_id);
        glDeleteShader(mvs_id);
        glDeleteShader(fs_id);
    }


    VertexArray soc(
            6,
            6,
            1,
            {{GL_LINES, 0, 6}}
    );
    {
        auto mapping = soc.vb.map();
        Vertex* const vertices = (Vertex*) mapping->buffer;
        vertices[0].position = glm::vec3(1, 0, 0);
        vertices[1].position = glm::vec3(-1, 0, 0);
        vertices[2].position = glm::vec3(0, 1, 0);
        vertices[3].position = glm::vec3(0, -1, 0);
        vertices[4].position = glm::vec3(0, 0, 1);
        vertices[5].position = glm::vec3(0, 0, -1);
    
        vertices[0].color = glm::u8vec4(255, 0, 0, 255);
        vertices[1].color = glm::u8vec4(0, 0, 255, 255);
        vertices[2].color = glm::u8vec4(255, 0, 0, 255);
        vertices[3].color = glm::u8vec4(0, 0, 255, 255);
        vertices[4].color = glm::u8vec4(255, 0, 0, 255);
        vertices[5].color = glm::u8vec4(0, 0, 255, 255);
    }

    {
        auto mapping = soc.eb.map();
        GLuint* const elements = (GLuint*) mapping->buffer;
        for( GLuint i = 0; i < soc.eb.size; i++ ) {
            elements[i] = i;
        }
    }

    {
        auto mapping = soc.imb.map();
        glm::mat4* const models = (glm::mat4*) mapping->buffer;
        models[0] = glm::scale(glm::mat4(1), glm::vec3(6));
    }


    VertexArray sphere(
            sphere_vertexbuffer_size(6),
            sphere_elementbuffer_size(6),
            1,
            sphere_elementbuffer_partitions(6)
    );
    {
        auto mapping = sphere.vb.map();
        Vertex* const vertices = (Vertex*) mapping->buffer;
        sphere_vertex_positions(vertices, 6);
        for( size_t i = 0; i < sphere.vb.size; i++ ) {
            vertices[i].color = glm::u8vec4(255, 255, 0, 255);
        }
    }

    {
        auto mapping = sphere.eb.map();
        GLuint* const elements = (GLuint*) mapping->buffer;
        sphere_elements(elements, 6);
    }

    {
        auto mapping = sphere.imb.map();
        glm::mat4* const models = (glm::mat4*) mapping->buffer;
        models[0] = glm::translate(glm::mat4(1), glm::vec3(2, 2, 0));
//        models[1] = glm::translate(glm::mat4(1), glm::vec3(0, -2, 2));
//        models[2] = glm::translate(glm::mat4(1), glm::vec3(0, 0, -2));
    }


    GLint sp_u_projection = glGetUniformLocation(sp_id, "projection");
    GLint sp_u_view = glGetUniformLocation(sp_id, "view");

    GLint mp_u_projection = glGetUniformLocation(mp_id, "projection");
    GLint mp_u_view = glGetUniformLocation(mp_id, "view");
    GLint mp_u_time = glGetUniformLocation(mp_id, "time");

    glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), 
            ((float) width) / height,
            4.0f,
            20.0f
    );
    glProgramUniformMatrix4fv(
            sp_id,
            sp_u_projection,
            1,
            GL_FALSE,
            glm::value_ptr(projection)
    );
    glProgramUniformMatrix4fv(
            mp_id,
            mp_u_projection,
            1,
            GL_FALSE,
            glm::value_ptr(projection)
    );

    glm::mat4 view = glm::lookAt(
            glm::vec3(3, 4, 10),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
    );
    glProgramUniformMatrix4fv(
            sp_id,
            sp_u_view,
            1,
            GL_FALSE,
            glm::value_ptr(view)
    );
    glProgramUniformMatrix4fv(
            mp_id,
            mp_u_view,
            1,
            GL_FALSE,
            glm::value_ptr(view)
    );


    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(RESTART_INDEX);

    glClearColor(0, 0, 0, 1);


    double spf = 1.0 / 60;
    double elapsed = 0;
    while( !glfwWindowShouldClose(window) ) {
        double frame_start = glfwGetTime();

        {  // drawing
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(sp_id);

            soc.draw();

            glUseProgram(mp_id);

            glUniform1f(mp_u_time, (float) elapsed);

            sphere.draw();

            glUseProgram(0);

            glfwSwapBuffers(window);
        }

        {  // event handling
            glfwPollEvents();
        }

        double time_surplus = spf - (glfwGetTime() - frame_start);
        if( time_surplus > 0 ) {
            std::this_thread::sleep_for(
                    std::chrono::milliseconds((size_t) (time_surplus * 1000))
            );
        }
        elapsed += spf;
    }

    defer.unfold();

    return EXIT_SUCCESS;
}

