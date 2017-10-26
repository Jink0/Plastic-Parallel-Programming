# Directory paths

BIN_DIR     = bin
BUILD_DIR   = build
INCLUDE_DIR = include
SRC_DIR     = src
UTILS_DIR   = utils
KERNELS_DIR = kernels

GCC       = g++
CXXFLAGS  = -Wall -std=c++11 -O3 $(flags)
INCLUDES  = -I$(INCLUDE_DIR)/ -I$(UTILS_DIR)/include -I$(KERNELS_DIR)/include
LIB_FLAGS = -lpthread



_JAC_OBJ = jacobi.o utils.o config_file_utils.o asm_kernels.o
JAC_OBJ  = $(patsubst %,$(BUILD_DIR)/%,$(_JAC_OBJ))



$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(GCC) -c -o $@ $< $(INCLUDES) $(CXXFLAGS)

$(BUILD_DIR)/%.o: $(UTILS_DIR)/src/%.cpp
	$(GCC) -c -o $@ $< $(INCLUDES) $(CXXFLAGS)

$(BUILD_DIR)/%.o: $(KERNELS_DIR)/src/%.cpp
	$(GCC) -c -o $@ $< $(INCLUDES) $(CXXFLAGS)



jacobi: $(JAC_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ -I$(INCLUDE_DIR) $(CXXFLAGS) $(LIB_FLAGS)

main: jacobi

all: jacobi

	

.PHONY: clean

clean:
	rm -f $(BUILD_DIR)/*.o $(BIN_DIR)/*
