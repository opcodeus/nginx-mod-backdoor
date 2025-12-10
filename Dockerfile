FROM ubuntu:latest
ENV DEBIAN_FRONTEND=noninteractive

RUN apt update -yqq 
RUN apt-get update && apt-get -y install build-essential libpcre3 libpcre3-dev zlib1g zlib1g-dev libssl-dev curl
COPY ngx_mod_template/ /root/ngx_mod_template
COPY entrypoint.sh /entrypoint.sh
ENTRYPOINT ["/bin/bash", "/entrypoint.sh", "$@"]
