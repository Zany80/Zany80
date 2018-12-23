# For travis
FROM buildpack-deps:xenial
SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive LANG=C.UTF-8
RUN mkdir -p /root/Oryolized/
COPY . /root/Oryolized/

RUN cd /root/Oryolized \
 && apt update \
 && apt install rsync cmake python3 build-essential -y \
 && ./tools/ensure_sdk.sh \
 && cd scas/build \
 && rm * .[!.]* -rf \
 && cmake .. && make -j8 && make install \
 && cd ../.. \
 && ./fips build
