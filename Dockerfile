# For travis
FROM buildpack-deps:xenial
SHELL ["/bin/bash", "-c"]
ENV DEBIAN_FRONTEND=noninteractive LANG=C.UTF-8
RUN mkdir -p /root/Zany80/
COPY . /root/Zany80/

RUN cd /root/Zany80 \
 && apt update \
 && apt install rsync cmake python3 build-essential -y \
 && ./tools/ensure_sdk.sh \
 && ./fips build
