# controller_test: controller_test.o comm.o strategy.o
# 	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@ $(LIBS)

# taskfarm_test: taskfarm_test.o task_farm.o \
# 	task_farm_threads.o tasks.o comm.o metrics.o arr_access.o
# 	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@ $(LIBS)

# tasks_test: tasks_test.o tasks.o
# 	$(CC) $(CFLAGS) $^ -o $(BIN_DIR)/$@ $(LIBS)

map_array_test: map_array_test.o metrics.o
	$(CXX) $(CPPFLAGS) $^ -o $(BIN_DIR)/$@