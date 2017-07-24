
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <unordered_map>
#include <cmath>
#include <functional>
#include <chrono>
#include <thread>


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
    
    GLuint sp_id = create_linked_program_with(svs_id, fs_id);
    GLuint mp_id = create_linked_program_with(mvs_id,fs_id); 
    
    glDeleteShader(svs_id);
    glDeleteShader(mvs_id);
    glDeleteShader(fs_id);

    GLint sp_u_projection = glGetUniformLocation(sp_id, "projection");
    GLint sp_u_view = glGetUniformLocation(sp_id, "view");
    GLint sp_a_position = glGetAttribLocation(sp_id, "position");
    GLint sp_a_color = glGetAttribLocation(sp_id, "color");

    GLint mp_u_projection = glGetUniformLocation(mp_id, "projection");
    GLint mp_u_view = glGetUniformLocation(mp_id, "view");
    GLint mp_u_time = glGetUniformLocation(mp_id, "time");
    GLint mp_a_position = glGetAttribLocation(mp_id, "position");
    GLint mp_a_color = glGetAttribLocation(mp_id, "color");
    GLint mp_a_model = glGetAttribLocation(mp_id, "model");

    GLuint soc_vao, soc_vbo;
    glCreateVertexArrays(1, &soc_vao);
    glCreateBuffers(1, &soc_vbo);

    const size_t soc_num_vertices = 6;
    {
        glNamedBufferStorage(
                soc_vbo,
                sizeof(Vertex) * soc_num_vertices,
                NULL,
                GL_MAP_WRITE_BIT
        );
        Vertex* const soc = (Vertex*) glMapNamedBuffer(
                soc_vbo,
                GL_WRITE_ONLY
        );
        soc[0].position = glm::vec3(1, 0, 0);
        soc[1].position = glm::vec3(-1, 0, 0);
        soc[2].position = glm::vec3(0, 1, 0);
        soc[3].position = glm::vec3(0, -1, 0);
        soc[4].position = glm::vec3(0, 0, 1);
        soc[5].position = glm::vec3(0, 0, -1);
    
        soc[0].color = glm::u8vec4(255, 0, 0, 255);
        soc[1].color = glm::u8vec4(0, 0, 255, 255);
        soc[2].color = glm::u8vec4(255, 0, 0, 255);
        soc[3].color = glm::u8vec4(0, 0, 255, 255);
        soc[4].color = glm::u8vec4(255, 0, 0, 255);
        soc[5].color = glm::u8vec4(0, 0, 255, 255);
        glUnmapNamedBuffer(soc_vbo);
    }

    glBindVertexArray(soc_vao);
    glBindBuffer(GL_ARRAY_BUFFER, soc_vbo);
    glEnableVertexAttribArray(sp_a_position);
    glVertexAttribPointer(
            sp_a_position,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(sp_a_color);
    glVertexAttribPointer(
            sp_a_color,
            4,
            GL_UNSIGNED_BYTE,
            GL_TRUE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, color)
    );
    glBindVertexArray(0);


    GLuint disc_vao, disc_vbo, disc_imbo;
    glCreateVertexArrays(1, &disc_vao);
    glCreateBuffers(1, &disc_vbo);
    glCreateBuffers(1, &disc_imbo);

    const size_t disc_num_vertices = 130;
    {
        glNamedBufferStorage(
                disc_vbo,
                sizeof(Vertex) * disc_num_vertices,
                NULL,
                GL_MAP_WRITE_BIT
        );
        Vertex* const disc = (Vertex*) glMapNamedBuffer(
                disc_vbo,
                GL_WRITE_ONLY
        );

        const size_t triangles = disc_num_vertices - 2;
        disc[0].position = glm::vec3(0, 0, 0);
        float segment_angle = (2 * M_PI) / triangles;
        for( size_t i = 1; i < disc_num_vertices; i++ ) {
            float angle = (i - 1) * segment_angle;
            disc[i].position = glm::vec3(
                    std::cos(angle),
                    std::sin(angle),
                    0
            );
        }
    
        disc[0].color = glm::u8vec4(255, 255, 255, 255);
        for( size_t i = 1; i < disc_num_vertices; i++ )
            disc[i].color = glm::u8vec4(255, 255, 0, 255);
        glUnmapNamedBuffer(disc_vbo);
    }

    const size_t disc_num_instances = 3;
    {
        glNamedBufferStorage(
                disc_imbo,
                sizeof(glm::mat4) * disc_num_instances,
                NULL,
                GL_MAP_WRITE_BIT
        );
        glm::mat4* const models = (glm::mat4*) glMapNamedBuffer(
                disc_imbo,
                GL_WRITE_ONLY
        );
        models[0] =
            glm::translate(glm::mat4(1), glm::vec3(2, -2, 0)) *
            glm::scale(glm::mat4(1), glm::vec3(0.5)) *
            glm::rotate(glm::mat4(1), glm::radians(15.0f), glm::vec3(0, 0, 1));
        models[1] =
            glm::translate(glm::mat4(1), glm::vec3(0, 0, 2)) *
            glm::scale(glm::mat4(1), glm::vec3(0.5)) *
            glm::rotate(glm::mat4(1), glm::radians(30.0f), glm::vec3(1, 0, 0));
        models[2] =
            glm::translate(glm::mat4(1), glm::vec3(0, 2, -2)) *
            glm::scale(glm::mat4(1), glm::vec3(0.5)) *
            glm::rotate(glm::mat4(1), glm::radians(45.0f), glm::vec3(0, 1, 0));
        glUnmapNamedBuffer(disc_imbo);
    }
    
    glBindVertexArray(disc_vao);

    glBindBuffer(GL_ARRAY_BUFFER, disc_vbo);
    glEnableVertexAttribArray(mp_a_position);
    glVertexAttribPointer(
            mp_a_position,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(mp_a_color);
    glVertexAttribPointer(
            mp_a_color,
            4,
            GL_UNSIGNED_BYTE,
            GL_TRUE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, color)
    );

    glBindBuffer(GL_ARRAY_BUFFER, disc_imbo);
    for( size_t i = 0; i < 4; i++ ) {
        glEnableVertexAttribArray(mp_a_model + i);
        glVertexAttribPointer(
                mp_a_model + i,
                4,
                GL_FLOAT,
                GL_FALSE,
                sizeof(glm::mat4),
                (void*) ((sizeof(glm::mat4) / 4) * i)
        );
        glVertexAttribDivisor(mp_a_model + i, 1);
    }

    glBindVertexArray(0);


    GLuint cube_vao, cube_ebo, cube_vbo, cube_imbo;
    glCreateVertexArrays(1, &cube_vao);
    glCreateBuffers(1, &cube_vbo);
    glCreateBuffers(1, &cube_ebo);
    glCreateBuffers(1, &cube_imbo);

    const size_t cube_num_vertices = 8;
    {
        glNamedBufferStorage(
                cube_vbo,
                sizeof(Vertex) * cube_num_vertices,
                NULL,
                GL_MAP_WRITE_BIT
        );
        Vertex* const cube = (Vertex*) glMapNamedBuffer(
                cube_vbo,
                GL_WRITE_ONLY
        );
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
        cube[4].color = glm::u8vec4(  0,   0, 255, 255);
        cube[5].color = glm::u8vec4(255, 255, 255, 255);
        cube[6].color = glm::u8vec4(255,   0,   0, 255);
        cube[7].color = glm::u8vec4(  0, 255,   0, 255);
        glUnmapNamedBuffer(cube_vbo);
    }

    const size_t cube_num_elements = 17;
    {
        const uint8_t elements_s[] = {
            0, 1, 2, 3, 4, 5, 6, 7,
            0xFF,
            2, 4, 0, 6, 1, 7, 3, 5
        };
        glNamedBufferStorage(
                cube_ebo,
                sizeof(uint8_t) * cube_num_elements,
                NULL,
                GL_MAP_WRITE_BIT
        );
        uint8_t* const elements_d = (uint8_t*) glMapNamedBuffer(
                cube_ebo,
                GL_WRITE_ONLY
        );
        std::memcpy(
                elements_d,
                elements_s,
                sizeof(uint8_t) * cube_num_elements
        );
        glUnmapNamedBuffer(cube_ebo);
    }

    const size_t cube_num_instances = 3;
    {
        glNamedBufferStorage(
                cube_imbo,
                sizeof(glm::mat4) * cube_num_instances,
                NULL,
                GL_MAP_WRITE_BIT
        );
        glm::mat4* const models = (glm::mat4*) glMapNamedBuffer(
                cube_imbo,
                GL_WRITE_ONLY
        );
        models[0] =
            glm::translate(glm::mat4(1), glm::vec3(2, 0, 0)) *
            glm::scale(glm::mat4(1), glm::vec3(0.25)) *
            glm::rotate(glm::mat4(1), glm::radians(15.0f), glm::vec3(1, 1, 0));
        models[1] =
            glm::translate(glm::mat4(1), glm::vec3(0, 2, 2)) *
            glm::scale(glm::mat4(1), glm::vec3(0.25)) *
            glm::rotate(glm::mat4(1), glm::radians(30.0f), glm::vec3(0, 1, 1));
        models[2] =
            glm::translate(glm::mat4(1), glm::vec3(0, -2, -2)) *
            glm::scale(glm::mat4(1), glm::vec3(0.25)) *
            glm::rotate(glm::mat4(1), glm::radians(45.0f), glm::vec3(1, 0, 1));
        glUnmapNamedBuffer(cube_imbo);
    }

    glBindVertexArray(cube_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);

    glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
    glEnableVertexAttribArray(mp_a_position);
    glVertexAttribPointer(
            mp_a_position,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(mp_a_color);
    glVertexAttribPointer(
            mp_a_color,
            4,
            GL_UNSIGNED_BYTE,
            GL_TRUE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, color)
    );

    glBindBuffer(GL_ARRAY_BUFFER, cube_imbo);
    for( size_t i = 0; i < 4; i++ ) {
        glEnableVertexAttribArray(mp_a_model + i);
        glVertexAttribPointer(
                mp_a_model + i,
                4,
                GL_FLOAT,
                GL_FALSE,
                sizeof(glm::mat4),
                (void*) ((sizeof(glm::mat4) / 4) * i)
        );
        glVertexAttribDivisor(mp_a_model + i, 1);
    }

    glBindVertexArray(0);


    GLuint octahedron_vao, octahedron_ebo, octahedron_vbo, octahedron_imbo;
    glCreateVertexArrays(1, &octahedron_vao);
    glCreateBuffers(1, &octahedron_vbo);
    glCreateBuffers(1, &octahedron_ebo);
    glCreateBuffers(1, &octahedron_imbo);

    const size_t octahedron_num_vertices = 6;
    {
        glNamedBufferStorage(
                octahedron_vbo,
                sizeof(Vertex) * octahedron_num_vertices,
                NULL,
                GL_MAP_WRITE_BIT
        );
        Vertex* const octahedron = (Vertex*) glMapNamedBuffer(
                octahedron_vbo,
                GL_WRITE_ONLY
        );
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
        glUnmapNamedBuffer(octahedron_vbo);
    }

    const size_t octahedron_num_elements = 13;
    {
        const uint8_t elements_s[] = {
            0, 1, 2, 3, 4, 1,
            0xFF,
            5, 1, 2, 3, 4, 1
        };
        glNamedBufferStorage(
                octahedron_ebo,
                sizeof(uint8_t) * octahedron_num_elements,
                NULL,
                GL_MAP_WRITE_BIT
        );
        uint8_t* const elements_d = (uint8_t*) glMapNamedBuffer(
                octahedron_ebo,
                GL_WRITE_ONLY
        );
        std::memcpy(
                elements_d,
                elements_s,
                sizeof(uint8_t) * octahedron_num_elements
        );
        glUnmapNamedBuffer(octahedron_ebo);
    }

    const size_t octahedron_num_instances = 3;
    {
        glNamedBufferStorage(
                octahedron_imbo,
                sizeof(glm::mat4) * octahedron_num_instances,
                NULL,
                GL_MAP_WRITE_BIT
        );
        glm::mat4* const models = (glm::mat4*) glMapNamedBuffer(
                octahedron_imbo,
                GL_WRITE_ONLY
        );
        models[0] =
            glm::translate(glm::mat4(1), glm::vec3(2, 2, 0)) *
            glm::scale(glm::mat4(1), glm::vec3(0.5)) *
            glm::rotate(glm::mat4(1), glm::radians(15.0f), glm::vec3(1, 0, 1));
        models[1] =
            glm::translate(glm::mat4(1), glm::vec3(0, -2, 2)) *
            glm::scale(glm::mat4(1), glm::vec3(0.5)) *
            glm::rotate(glm::mat4(1), glm::radians(30.0f), glm::vec3(0, 1, 1));
        models[2] =
            glm::translate(glm::mat4(1), glm::vec3(0, 0, -2)) *
            glm::scale(glm::mat4(1), glm::vec3(0.5)) *
            glm::rotate(glm::mat4(1), glm::radians(45.0f), glm::vec3(1, 1, 0));
        glUnmapNamedBuffer(octahedron_imbo);
    }

    glBindVertexArray(octahedron_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, octahedron_ebo);

    glBindBuffer(GL_ARRAY_BUFFER, octahedron_vbo);
    glEnableVertexAttribArray(mp_a_position);
    glVertexAttribPointer(
            mp_a_position,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, position)
    );
    glEnableVertexAttribArray(mp_a_color);
    glVertexAttribPointer(
            mp_a_color,
            4,
            GL_UNSIGNED_BYTE,
            GL_TRUE,
            sizeof(Vertex),
            (void*) offsetof(Vertex, color)
    );

    glBindBuffer(GL_ARRAY_BUFFER, octahedron_imbo);
    for( size_t i = 0; i < 4; i++ ) {
        glEnableVertexAttribArray(mp_a_model + i);
        glVertexAttribPointer(
                mp_a_model + i,
                4,
                GL_FLOAT,
                GL_FALSE,
                sizeof(glm::mat4),
                (void*) ((sizeof(glm::mat4) / 4) * i)
        );
        glVertexAttribDivisor(mp_a_model + i, 1);
    }

    glBindVertexArray(0);


    glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), 
            ((float) width) / height,
            0.1f,
            16.0f
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


    glEnable(GL_DEPTH_TEST);

    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(0xFF);

    glClearColor(0, 0, 0, 1);


    double spf = 1.0 / 60;
    double elapsed = 0;
    while( !glfwWindowShouldClose(window) ) {
        double frame_start = glfwGetTime();

        {  // drawing
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(sp_id);

            glBindVertexArray(soc_vao);
            glDrawArrays(
                    GL_LINES,
                    0,
                    soc_num_vertices
            );
            glBindVertexArray(0);

            glUseProgram(mp_id);

            glUniform1f(mp_u_time, (float) elapsed);

            glBindVertexArray(disc_vao);
            glDrawArraysInstanced(
                    GL_TRIANGLE_FAN,
                    0,
                    disc_num_vertices,
                    disc_num_instances
            );
            glBindVertexArray(0);

            glBindVertexArray(cube_vao);
            glDrawElementsInstanced(
                    GL_TRIANGLE_STRIP,
                    cube_num_elements,
                    GL_UNSIGNED_BYTE,
                    NULL,
                    cube_num_instances
            );
            glBindVertexArray(0);

            glBindVertexArray(octahedron_vao);
            glDrawElementsInstanced(
                    GL_TRIANGLE_FAN,
                    octahedron_num_elements,
                    GL_UNSIGNED_BYTE,
                    NULL,
                    octahedron_num_instances
            );
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
        elapsed += spf;
    }

    return EXIT_SUCCESS;
}

