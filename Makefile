BUILD_DIR = build
NUM_CORES = $(shell getconf _NPROCESSORS_ONLN)
GENERATOR = $(shell which ninja &> /dev/null && echo Ninja || echo \"Unix Makefiles\")
OPTIONS = -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build:
	cmake --build $(BUILD_DIR) -- -j $(NUM_CORES)

cmake:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake $(OPTIONS) -G$(GENERATOR) ..

clean:
	rm -rf $(BUILD_DIR)

all: clean cmake build

.PHONY: all clean cmake build
