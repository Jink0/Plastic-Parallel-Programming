CONTROLLER_DIR  = ../src/controller
MAP_ARRAY_DIR   = ../src/map_array
TEST_DIR        = ../src/test

BIN_DIR         = ../bin
DEPENDENCY_FILE = ./dependencies.d

vpath %.cpp $(CONTROLLER_DIR) $(MAP_ARRAY_DIR) $(TEST_DIR)
vpath %.h $(MAP_ARRAY_DIR)

CPPFLAGS += -Wall -pthread -std=c++11 -I$(MAP_ARRAY_DIR) -g

LIBS += -lboost_system -lboost_filesystem

-include $(DEPENDENCY_FILE)

include $(CONTROLLER_DIR)/makefile
include $(TEST_DIR)/makefile

.PHONY: dependencies clean

dependencies:
	$(CXX) -MM -std=c++11 -I$(MAP_ARRAY_DIR) $(wildcard ../src/controller/*.cpp) > $(DEPENDENCY_FILE)
	$(CXX) -MM -std=c++11 -I$(MAP_ARRAY_DIR) $(wildcard ../src/map_array/*.cpp)  >> $(DEPENDENCY_FILE)
	$(CXX) -MM -std=c++11 -I$(MAP_ARRAY_DIR) $(wildcard ../src/test/*.cpp) 		 >> $(DEPENDENCY_FILE)

clean:
	rm -rf *.o && rm -rf ../bin/*
