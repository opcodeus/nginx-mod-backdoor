#!/bin/bash
set -euo pipefail

docker build -t nginx-backdoor-mod .
docker run --rm -it -v $(pwd)/nginx-backdoor-mod:/root/nginx-backdoor-mod nginx-backdoor-mod /bin/bash /root/build-nginx-backdoor-mod.sh "$@"
