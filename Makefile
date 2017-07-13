CC=gcc
SOURCES=main.c
TARGET=main

# optional as of yet: -lfreeglut -lglu32 -mwindows

all: $(SOURCES)
	gcc $(SOURCES) -lSDL2main -lSDL2 -lglew32 -lfreeglut -lopengl32 -lglu32 -mwindows -o $(TARGET)

.PHONY: clean
clean:
	-rm $(TARGET) *.exe *~

re: clean all

run: all
	./$(TARGET)

rerun: re run

