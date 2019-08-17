PYTHON_VERSION=3.7
WEBRTC_LIBRARY=/Users/itq/code/pywebrtc/third_party

build: 
	mkdir -p build && \
	cd build && \
	cmake -DCMAKE_BUILD_TYPE=Debug \
		  -DLIBWEBRTC_PATH=${WEBRTC_LIBRARY} .. && \
	$(MAKE) 
	
test: build
	python build/core/websocket_server.py &
	python build/core/websocket_client.py
clean:
	rm -rf build


.PHONY: build
