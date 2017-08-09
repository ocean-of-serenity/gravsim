
CC=clang++
SOURCES=main.cc
TARGET=main.exe

SHADERS=static_vertex_shader.glsl moving_vertex_shader.glsl fragment_shader.glsl

$(TARGET): $(SOURCES)
	$(CC) $(SOURCES) -o $(TARGET) -std=c++14 -fno-exceptions -Wall -Werror \
		-mwindows -DGLEW_STATIC -lglew32 -lopengl32 \
		-static -lglfw3 -lgcc -lstdc++

all: $(TARGET)

run: all
	./$(TARGET)

.PHONY: clean
clean:
	-rm $(TARGET)

re: clean all

rerun: re run

pkg: $(TARGET) $(SHADERS)
	zip `basename ${PWD}` $(TARGET) $(SHADERS)

