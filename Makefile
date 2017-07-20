
CC=g++
SOURCES=main.cc
TARGET=main

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(SOURCES) -lglew32 -lglfw3 -lopengl32 -mwindows -o $(TARGET)

.PHONY: clean
clean:
	-rm $(TARGET)

re: clean all

run: $(TARGET)
	./$(TARGET)

rerun: re run

