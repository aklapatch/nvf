CFLAGS := -O2
BUILD_DIR := build/

all: $(BUILD_DIR)nvf_test $(BUILD_DIR)libnvf.a

test: $(BUILD_DIR)nvf_test
	./$(BUILD_DIR)nvf_test

$(BUILD_DIR)nvf_test: nvf_test.c $(BUILD_DIR)libnvf_debug.a
	$(CC) -g3 $(CFLAGS) $^ -o $@

$(BUILD_DIR)libnvf.a: $(BUILD_DIR)nvf.o
	$(AR) rcs $@ $<

$(BUILD_DIR)nvf.o: nvf.c nvf.h $(BUILD_DIR)
	$(CC) $(CFLAGS)  -c $< -o $@

$(BUILD_DIR)libnvf_debug.a: $(BUILD_DIR)nvf_debug.o
	$(AR) rcs $@ $<

$(BUILD_DIR)nvf_debug.o: nvf.c nvf.h $(BUILD_DIR)
	$(CC) $(CFLAGS) -g3 -c $< -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

clean:
	rm -fr $(BUILD_DIR)
