PYTHON_VERSION=3.7
PYTHON_LIBRARY=/usr/local/Frameworks/Python.framework/Versions/$(PYTHON_VERSION)/lib/libpython$(PYTHON_VERSION).dylib
PYTHON_INCLUDE_DIR=/usr/local/Frameworks/Python.framework/Versions/$(PYTHON_VERSION)/Headers/
WEBRTC_LIBRARY=/Users/itq/code/pywebrtc/third_party

build: 
	mkdir -p build && \
	cd build && \
	cmake -DPYTHON_LIBRARY=$(PYTHON_LIBRARY) \
		  -DPYTHON_INCLUDE_DIR=$(PYTHON_INCLUDE_DIR) \
		  -DCMAKE_BUILD_TYPE=$(DEBUG) \
		  -DLIBWEBRTC_PATH=${WEBRTC_LIBRARY} .. && \
	$(MAKE) 
	
test: build
	python build/core/websocket_server.py &
	python build/core/websocket_client.py
clean:
	rm -rf build


.PHONY: build
