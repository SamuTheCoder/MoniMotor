# Get compile/link flags from sdl2-config to assure portability
SDL2_CONFIG = sdl2-config
CFLAGS = $(shell $(SDL2_CONFIG) --cflags)
LDFLAGS = $(shell $(SDL2_CONFIG) --libs)

# Add other flags if necessary
LDFLAGS += -lm  
CFLAGS += -g

# Define the target and source files
TARGET = monimotor
SOURCES = monimotor.c ./fft/fft.c cab_buffer.c 
HEADERS = ./include
OBJECTS = $(SOURCES:.c=.o)

CC = gcc

# Main target
all: $(TARGET)

# Linking target
$(TARGET): $(OBJECTS) 
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Compilation target
%.o: %.c
	$(CC) $(CFLAGS) -I $(HEADERS) -c $< -o $@

# Clean up the build
clean:
	rm -f *.o $(TARGET) $(OBJECTS)

# Run the program
run: $(TARGET)
	clear
	./$(TARGET) 0