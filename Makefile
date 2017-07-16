
CC=g++
SOURCES=main.cc
TARGET=main


all: $(SOURCES)
	$(CC) $(SOURCES) -lglew32 -lglfw3 -lopengl32 -mwindows -o $(TARGET)

.PHONY: clean
clean:
	-rm $(TARGET)

re: clean all

run: all
	./$(TARGET)

rerun: re run

