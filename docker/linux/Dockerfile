FROM ubuntu:24.04

RUN apt-get update && \
		apt-get install -y \
				build-essential \
                gcc-14 \
                g++-14 \
				cmake  \
				ninja-build \
		&& rm  -rf /var/lib/apt/lists/*

ENV CC=gcc-14
ENV CXX=g++-14

WORKDIR /workspace

COPY . .

RUN cmake -S . -B build -G Ninja \
		-DCMAKE_BUILD_TYPE=Debug
	
RUN cmake --build  build

RUN cd build && ctest --output-on-failure

CMD ["./build/bermudadb"]
