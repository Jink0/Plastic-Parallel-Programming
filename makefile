# Directory paths

BIN_DIR             = bin
BUILD_DIR           = build
INCLUDE_DIR         = include
SRC_DIR             = src
MAP_ARRAY_TEST_DIR	= test/map_array_test
PARALLEL_TEST_DIR   = test/parallel_test
SEQUENTIAL_TEST_DIR = test/sequential_test
UTILS_DIR           = utils

# Flags and includes

GCC       = g++
CXXFLAGS  = -Wall -std=c++11 -std=c++1y -pthread -fopenmp -O3 -DDETAILED_METRICS -DCONTROLLER -g
INCLUDES  = -I$(INCLUDE_DIR) -I$(UTILS_DIR)/include -I$(MAP_ARRAY_TEST_DIR)/include -I$(PARALLEL_TEST_DIR)/include -I$(SEQUENTIAL_TEST_DIR)/include
LIB_FLAGS = -lboost_system -lboost_filesystem -lboost_thread -lzmq -ltbb



_CON_OBJ = controller.o
CON_OBJ  = $(patsubst %,$(BUILD_DIR)/%,$(_CON_OBJ))

_MAT_OBJ = map_array_test.o utils.o config_files_utils.o workloads.o metrics.o
MAT_OBJ  = $(patsubst %,$(BUILD_DIR)/%,$(_MAT_OBJ))

_PAR_OBJ = parallel_test.o utils.o config_files_utils.o workloads.o metrics.o
PAR_OBJ  = $(patsubst %,$(BUILD_DIR)/%,$(_PAR_OBJ))

_SEQ_OBJ = sequential_test.o utils.o config_files_utils.o workloads.o metrics.o
SEQ_OBJ  = $(patsubst %,$(BUILD_DIR)/%,$(_SEQ_OBJ))



$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(GCC) -c -o $@ $< $(INCLUDES) $(CXXFLAGS)

$(BUILD_DIR)/%.o: $(MAP_ARRAY_TEST_DIR)/src/%.cpp
	$(GCC) -c -o $@ $< $(INCLUDES) $(CXXFLAGS)

$(BUILD_DIR)/%.o: $(PARALLEL_TEST_DIR)/$(SRC_DIR)/%.cpp
	$(GCC) -c -o $@ $< $(INCLUDES) $(CXXFLAGS)

$(BUILD_DIR)/%.o: $(SEQUENTIAL_TEST_DIR)/$(SRC_DIR)/%.cpp
	$(GCC) -c -o $@ $< $(INCLUDES) $(CXXFLAGS)

$(BUILD_DIR)/%.o: $(UTILS_DIR)/src/%.cpp
	$(GCC) -c -o $@ $< $(INCLUDES) $(CXXFLAGS)



controller:      $(CON_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ -I$(UTILS_DIR)/include $(CXXFLAGS) $(LIB_FLAGS)

map_array_test:  $(MAT_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ -I$(INCLUDE_DIR) $(CXXFLAGS) $(LIB_FLAGS)

parallel_test:   $(PAR_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ $(CXXFLAGS) $(LIB_FLAGS)

sequential_test: $(SEQ_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ $(CXXFLAGS) $(LIB_FLAGS)

main: map_array_test controller

all: map_array_test controller sequential_test parallel_test

	

.PHONY: clean

clean:
	rm -f $(BUILD_DIR)/*.o $(BIN_DIR)/*