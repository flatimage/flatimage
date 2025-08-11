#!/usr/bin/env bash

set -e

#cd ..

docker build . --build-arg FIM_DIST=ARCH -t flatimage:arch -f ./docker/Dockerfile.boot

# docker run -it --rm -e FIM_DIST=ALPINE -v "$(pwd)":/workdir fim:latest cp /fim/alpine.fim /workdir/test
#docker run -it --rm -v "$(pwd)":/workdir fim:latest cp /fim/arch.fim /workdir/test
# docker run -it --rm -e FIM_DIST=UBUNTU -v "$(pwd)":/workdir fim:latest cp /fim/focal.fim /workdir/test

# sudo chown $(id -u):$(id -g) test/alpine.fim
#sudo chown $(id -u):$(id -g) test/arch.fim
# sudo chown $(id -u):$(id -g) test/focal.fim
