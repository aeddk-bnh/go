FROM ubuntu:24.04
RUN apt-get update && apt-get install -y \
    build-essential \
    ca-certificates \
    curl \
    python3 \
    python3-pip \
    cmake \
    ninja-build \
    unzip \
 && rm -rf /var/lib/apt/lists/*

RUN python3 -m pip install --no-cache-dir conan

WORKDIR /workspace
COPY . /workspace
CMD ["/bin/bash"]
