
CC=g++
SOURCES=main.cc
TARGET=main

$(TARGET): $(SOURCES)
	$(CC) $(SOURCES) -lglew32 -lglfw3 -lopengl32 -mwindows -o $(TARGET)

all: $(TARGET)

run: all
	./$(TARGET)

.PHONY: clean
clean:
	-rm $(TARGET)

re: clean all

rerun: re run

