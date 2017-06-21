# Directory paths

BIN_DIR             = bin
BUILD_DIR           = build
INCLUDE_DIR         = include
SRC_DIR             = src
MAP_ARRAY_TEST_DIR	= test/map_array_test
PARALLEL_TEST_DIR   = test/parallel_test
SEQUENTIAL_TEST_DIR = test/sequential_test
UTILS_DIR           = utils



GCC       = g++
CPPFLAGS  = -Wall -pthread -std=c++11 -I$(INCLUDE_DIR) -DMETRICS -fopenmp -O3
LIB_FLAGS = -lboost_filesystem -lboost_system -lboost_thread -lzmq



# _DEPS = map_array_test.hpp map_array_test_utils.hpp utils.hpp
# DEPS  = $(patsubst %,$(INCLUDE_DIR)/%,$(_DEPS))



_CON_OBJ = controller.o 
CON_OBJ    = $(patsubst %,$(BUILD_DIR)/%,$(_CON_OBJ))

_MAT_OBJ = map_array_test.o utils.o map_array_test_utils.o metrics.o workloads.o
MAT_OBJ  = $(patsubst %,$(BUILD_DIR)/%,$(_MAT_OBJ))

_PAR_OBJ = parallel_test.o workloads.o metrics.o utils.o
PAR_OBJ  = $(patsubst %,$(BUILD_DIR)/%,$(_PAR_OBJ))

_SEQ_OBJ = sequential_test.o workloads.o metrics.o utils.o
SEQ_OBJ  = $(patsubst %,$(BUILD_DIR)/%,$(_SEQ_OBJ))



$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp #$(DEPS)
	$(GCC) -c -o $@ $< -I$(UTILS_DIR)/include $(CPPFLAGS)

$(BUILD_DIR)/%.o: $(MAP_ARRAY_TEST_DIR)/src/%.cpp #$(DEPS)
	$(GCC) -c -o $@ $< -I$(MAP_ARRAY_TEST_DIR)/include -I$(UTILS_DIR)/include $(CPPFLAGS)

$(BUILD_DIR)/%.o: $(PARALLEL_TEST_DIR)/$(SRC_DIR)/%.cpp #$(DEPS)
	$(GCC) -c -o $@ $< -I$(PARALLEL_TEST_DIR)/include -I$(UTILS_DIR)/include $(CPPFLAGS)

$(BUILD_DIR)/%.o: $(SEQUENTIAL_TEST_DIR)/$(SRC_DIR)/%.cpp #$(DEPS)
	$(GCC) -c -o $@ $< -I$(SEQUENTIAL_TEST_DIR)/include -I$(UTILS_DIR)/include $(CPPFLAGS)

$(BUILD_DIR)/%.o: $(UTILS_DIR)/src/%.cpp #$(DEPS)
	$(GCC) -c -o $@ $< -I$(UTILS_DIR)/include $(CPPFLAGS)



controller:      $(CON_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ -I$(UTILS_DIR)/include $(CPPFLAGS) $(LIB_FLAGS)

map_array_test:  $(MAT_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ $(CPPFLAGS) $(LIB_FLAGS)

parallel_test:   $(PAR_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ $(CPPFLAGS) $(LIB_FLAGS)

sequential_test: $(SEQ_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ $(CPPFLAGS) $(LIB_FLAGS)

all: map_array_test controller sequential_test parallel_test

	

.PHONY: clean

clean:
	rm -f $(BUILD_DIR)/*.o $(BIN_DIR)/*