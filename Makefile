#CFLAGS=-DDEBUG
LDFLAGS=-pthread 
CC=gcc
OBJECTS=ecu_main.o ecu_dist.o ecu_enc.o ecu_merger.o
TARGET=encryptUtil

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	
clean :
	rm ./$(TARGET) *.o
