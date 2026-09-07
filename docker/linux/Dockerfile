FROM ubuntu:24.04

RUN apt-get update && \
		apt-get install -y \
				build-essential \
				cmake  \
				ninja-build \
		&& rm  -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY . .

RUN cmake -S . -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Debug
	
RUN cmake --build  build

RUN cd build && ctest --output-on-failure

CMD ["./build/bermudadb"]
