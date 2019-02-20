
CC=go build
NAME=main
SOURCES=$(NAME).go
TARGET=$(NAME).exe


$(TARGET): $(SOURCES)
	$(CC) -o $(TARGET) -ldflags '-H windowsgui' $(SOURCES)

all: $(TARGET)

run: all
	./$(TARGET)

.PHONY: clean
clean:
	-rm $(TARGET)

re: clean all

rerun: re run

pkg: $(TARGET)
	zip $(NAME).zip $(TARGET) *.glsl

