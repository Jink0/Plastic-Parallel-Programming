SRC_DIR   = ../src
UTILS_DIR = ../utils

INCLUDE_DIR = ../include

BIN_DIR    = ../bin
OBJECT_DIR = obj



GCC       = g++
CPPFLAGS  = -Wall -pthread -std=c++11 -I$(INCLUDE_DIR) -g -DMETRICS
LIB_FLAGS = -lboost_filesystem -lboost_system -lboost_thread -lzmq



#_DEPS = map_array_test.hpp utils.hpp
#DEPS  = $(patsubst %,$(INCLUDE_DIR)/%,$(_DEPS))



_MAT_OBJ = map_array_test.o utils.o metrics.o workloads.o
MAT_OBJ  = $(patsubst %,$(OBJECT_DIR)/%,$(_MAT_OBJ))

_C_OBJ = controller.o
C_OBJ  = $(patsubst %,$(OBJECT_DIR)/%,$(_C_OBJ))



$(OBJECT_DIR)/%.o: $(UTILS_DIR)/%.cpp #$(DEPS)
	$(GCC) -c -o $@ $< $(CPPFLAGS)

$(OBJECT_DIR)/%.o: $(SRC_DIR)/%.cpp #$(DEPS)
	$(GCC) -c -o $@ $< $(CPPFLAGS)



map_array_test: $(MAT_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ $(CPPFLAGS) $(LIB_FLAGS)

controller:  $(C_OBJ)
	$(GCC) -o $(BIN_DIR)/$@ $^ $(CPPFLAGS) $(LIB_FLAGS)



.PHONY: clean

clean:
	rm -f $(OBJECT_DIR)/*.o $(BIN_DIR)/*