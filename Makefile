PYTHON_VERSION=3.7
PYTHON_LIBRARY=/usr/local/Frameworks/Python.framework/Versions/$(PYTHON_VERSION)/lib/libpython$(PYTHON_VERSION).dylib
PYTHON_INCLUDE_DIR=/usr/local/Frameworks/Python.framework/Versions/$(PYTHON_VERSION)/Headers/
all:
	mkdir -p build && \
	cd build && \
	cmake -DPYTHON_LIBRARY=$(PYTHON_LIBRARY) \
		  -DPYTHON_INCLUDE_DIR=$(PYTHON_INCLUDE_DIR) \
		  -DCMAKE_BUILD_TYPE=$(DEBUG) ..  && \
	make 
clean:
	rm -rf build
