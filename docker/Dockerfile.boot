FROM alpine:latest

# Update repositories to edge
RUN echo https://dl-cdn.alpinelinux.org/alpine/edge/main/ > /etc/apk/repositories
RUN echo https://dl-cdn.alpinelinux.org/alpine/edge/community/ >> /etc/apk/repositories
RUN echo https://dl-cdn.alpinelinux.org/alpine/edge/testing/ >> /etc/apk/repositories

# Install deps
RUN apk update && apk upgrade
RUN apk add --no-cache build-base git libbsd-dev cmake clang clang-dev \
  make e2fsprogs-dev e2fsprogs-libs e2fsprogs-static libcom_err musl musl-dev \
  bash pcre-tools boost-dev libjpeg-turbo-dev libjpeg-turbo-static libpng-dev \
  libpng-static zlib-static upx nlohmann-json

# Copy boot directory
ARG FIM_DIR
COPY . $FIM_DIR
WORKDIR $FIM_DIR

ARG FIM_DIST
ENV FIM_DIST=$FIM_DIST

ARG FIM_RESERVED_SIZE
ENV FIM_RESERVED_SIZE=$FIM_RESERVED_SIZE

# Compile
RUN cmake -H. -Bbuild -DFIM_RESERVED_SIZE="$FIM_RESERVED_SIZE"
RUN cmake --build build
RUN strip -s ./build/boot

# Include magic bytes
RUN ./build/magic ./build/boot

# Compile janitor
RUN g++ --std=c++23 -O3 -static -o src/janitor/fim_janitor src/janitor/janitor.cpp
RUN strip -s src/janitor/fim_janitor
RUN upx -6 --no-lzma src/janitor/fim_janitor