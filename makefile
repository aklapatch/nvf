CFLAGS := -O2
BUILD_DIR := ./build/

all: $(BUILD_DIR)nvf_test $(BUILD_DIR)libnvf.a

doc: nvf.h
	
fmt: $(wildcard *.c) $(wildcard *.h)
	clang-format -i *.c *.h

debug: $(BUILD_DIR)nvf_test
	gdb $(BUILD_DIR)nvf_test

valgrind: $(BUILD_DIR)nvf_test
	valgrind --leak-check=full $<

example: $(BUILD_DIR)nvf_example
	$< 

test: $(BUILD_DIR)nvf_test
	$<

$(BUILD_DIR)nvf_test: nvf_test.c $(BUILD_DIR)libnvf_debug.a 
	$(CC) -g3 $^ -o $@

$(BUILD_DIR)nvf_example: nvf_example.c $(BUILD_DIR)libnvf.a
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)libnvf.a: $(BUILD_DIR)nvf.o
	$(AR) rcs $@ $<

$(BUILD_DIR)nvf.o: nvf.c nvf.h $(BUILD_DIR)
	$(CC) $(CFLAGS)  -c $< -o $@

$(BUILD_DIR)libnvf_debug.a: $(BUILD_DIR)nvf_debug.o
	$(AR) rcs $@ $<

$(BUILD_DIR)nvf_debug.o: nvf.c nvf.h $(BUILD_DIR)
	$(CC) -g3 -c $< -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

clean:
	rm -fr $(BUILD_DIR)
