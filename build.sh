#!/bin/bash

docker build -t nginx-backdoor-mod -f - . << "EOF"
FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive

RUN apt update -yqq 
RUN apt-get update && apt-get -y install build-essential libpcre3 libpcre3-dev zlib1g zlib1g-dev libssl-dev curl
COPY build-nginx-backdoor-mod.sh /root/build-nginx-backdoor-mod.sh
COPY ngx_mod_template/ /root/ngx_mod_template
WORKDIR /root
EOF

docker run --rm -it -v $(pwd)/nginx-backdoor-mod:/root/nginx-backdoor-mod nginx-backdoor-mod /bin/bash /root/build-nginx-backdoor-mod.sh "$@"
