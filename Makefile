COMPILE_FLAGS = -Wall --std=gnu99 -O2
#COMPILE_FLAGS = -Wall -Werror --std=gnu99 -O0
ifdef PROFILE
	COMPILE_FLAGS += -pg
endif
ifdef DEBUG
	COMPILE_FLAGS += -g
endif
#LIB_FLAGS = -lFLAC -lasound -lm -lpthread 
#LIB_FLAGS = -lasound -lm -lpthread
LIB_FLAGS = -lasound -lFLAC -lid3tag -lz -lmad -lm -lpthread
HEADERS_DIR = headers/
VPATH+=$(HEADERS_DIR)

#CXX:=clang $(COMPILE_FLAGS) -I$(HEADERS_DIR)
CXX:=gcc $(COMPILE_FLAGS) -I$(HEADERS_DIR)
#CXX:=aarch64-buildroot-linux-gnu-gcc $(COMPILE_FLAGS) -I$(HEADERS_DIR)

SOURCE_DIR = src/
OBJ_DIR = build/
MODULES = modules/
#MODULES = armlib/

VPATH+=$(SOURCE_DIR)
VPATH+=$(OBJ_DIR)

SOURCES_C := \
	src/flac.c \
	src/mp3.c \
	src/wav.c \
	src/ring_buffera.c \
	src/alsa.c 
#	src/soundio.c \


HEADERS := \
	headers/flac.h \
	headers/mp3.h \
	headers/ring_buffer.h \
	headers/wav.h \
	headers/constants.h \
	headers/alsa.h

OBJECTS :=${SOURCES_C:src/%.c=$(OBJ_DIR)%.o} 

all: player

.PHONY: build_dir

build_dir: $(OBJ_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

build/%.o: $(SOURCE_DIR)%.c build_dir
	$(CXX) $< -c -o $@ 
player: $(OBJECTS) $(SOURCES_C) $(HEADERS) 
	$(CXX) -c $(SOURCE_DIR)main.c -o $(OBJ_DIR)main.o
	$(CXX) -no-pie $(OBJECTS)  \
		$(OBJ_DIR)main.o $(LIB_FLAGS) -o player_dev

player_static: $(OBJECTS) $(SOURCES_C) $(HEADERS) 
	$(CXX) -c $(SOURCE_DIR)main.c -o $(OBJ_DIR)main.o
	$(CXX) -no-pie $(OBJECTS) $(MODULES)libFLAC.a $(MODULES)libmad.a \
		$(MODULES)libid3tag.a $(MODULES)libz.a $(MODULES)libogg.a \
		$(OBJ_DIR)main.o $(LIB_FLAGS) -o player_static


player_mac: $(OBJECTS) $(SOURCES_C) $(HEADERS) 
	$(CXX) -c $(SOURCE_DIR)main.c -o $(OBJ_DIR)main.o
	$(CXX) -no-pie $(OBJECTS) \
		$(OBJ_DIR)main.o -lsoundio -lintl $(LIB_FLAGS) -o player_dev

player_aarch64: $(OBJECTS) $(SOURCES_C) $(HEADERS) 
	$(CXX) -c $(SOURCE_DIR)main.c -o $(OBJ_DIR)main.o
	$(CXX) $(OBJECTS) \
		$(MODULES)libmad.a \
		$(MODULES)libFLAC.a $(MODULES)libogg.a \
		$(MODULES)libid3tag.a $(MODULES)libz.a \
		$(OBJ_DIR)main.o $(LIB_FLAGS) -o player_static_arm64

eq:
	$(CXX) -c $(SOURCE_DIR)eq.c -o eq.o

plurl:
	gcc -O2 -o plurl plurl.c -lmpg123 -lao -lcurl

unittest: $(OBJECTS) $(SOURCES_C) $(HEADERS)
	$(CXX) -c $(SOURCE_DIR)unittest.c -o $(OBJ_DIR)unittest.o
	$(CXX) $(OBJECTS) $(LIB_FLAGS) -lFLAC -lid3tag -lmad $(OBJ_DIR)unittest.o -o $(OBJ_DIR)unittest
	$(OBJ_DIR)unittest

rebuild: clean player

clean:
	rm -rf $(OBJ_DIR) player_dev
