compile:
	arduino-cli compile --fqbn esp32:esp32:esp32 -u -p /dev/ttyUSB0 ./

compile-test:
	cmake -S . -B build -Wno-author
	cmake --build build

run-tests:
	ctest --test-dir build --output-on-failure

compile-lsp:
	cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
