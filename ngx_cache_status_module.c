
/*
 * Copyright (C) Viktor Suprun
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#if !defined TOTAL_REQUESTS_SLOT
        #define TOTAL_REQUESTS_SLOT         0
#endif
#if !defined UNCACHED_REQUESTS_SLOT
        #define UNCACHED_REQUESTS_SLOT      1
#endif
#if !defined NGX_HTTP_CACHE_MISS_SLOT
        #define NGX_HTTP_CACHE_MISS_SLOT    2
#endif
#if !defined NGX_HTTP_CACHE_BYPASS
        #define NGX_HTTP_CACHE_BYPASS       3
#endif
#if !defined NGX_HTTP_CACHE_EXPIRED
        #define NGX_HTTP_CACHE_EXPIRED      4
#endif
#if !defined NGX_HTTP_CACHE_STALE
        #define NGX_HTTP_CACHE_STALE        5
#endif
#if !defined NGX_HTTP_CACHE_UPDATING
        #define NGX_HTTP_CACHE_UPDATING     6
#endif
#if !defined NGX_HTTP_CACHE_REVALIDATED
        #define NGX_HTTP_CACHE_REVALIDATED  7
#endif
#if !defined NGX_HTTP_CACHE_HIT
        #define NGX_HTTP_CACHE_HIT          8
#endif
#if !defined MISC_SLOT
        #define MISC_SLOT                   9
#endif

static ngx_atomic_uint_t cache_status[] = {
    0, //TOTAL_REQUESTS_SLOT
    0, //UNCACHED_REQUESTS_SLOT
    0, //NGX_HTTP_CACHE_MISS,
    0, //NGX_HTTP_CACHE_BYPASS
    0, //NGX_HTTP_CACHE_EXPIRED
    0, //NGX_HTTP_CACHE_STALE
    0, //NGX_HTTP_CACHE_UPDATING
    0, //NGX_HTTP_CACHE_REVALIDATED
    0, //NGX_HTTP_CACHE_HIT
    0  //MISC_SLOT
};

static ngx_http_output_header_filter_pt  ngx_original_filter_ptr;

static char *ngx_set_status(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_cache_status_filter(ngx_http_request_t *r);
static ngx_int_t ngx_cache_status_filter_init(ngx_conf_t *cf);

static ngx_command_t  ngx_status_commands[] = {

    { ngx_string("cache_status"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,
      ngx_set_status,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_cache_status_module_ctx = {
    NULL,				   /* preconfiguration */
    ngx_cache_status_filter_init,          /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_cache_status_module = {
    NGX_MODULE_V1,
    &ngx_cache_status_module_ctx,          /* module context */
    ngx_status_commands,                   /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_status_handler(ngx_http_request_t *r)
{
    size_t       size;
    ngx_int_t    rc;
    ngx_buf_t    *b;
    ngx_chain_t  out;

    if (r->method != NGX_HTTP_GET && r->method != NGX_HTTP_HEAD) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    rc = ngx_http_discard_request_body(r);

    if (rc != NGX_OK) {
        return rc;
    }

    r->headers_out.content_type_len = sizeof("text/plain") - 1;
    ngx_str_set(&r->headers_out.content_type, "text/plain");
    r->headers_out.content_type_lowcase = NULL;

    if (r->method == NGX_HTTP_HEAD) {
        r->headers_out.status = NGX_HTTP_OK;

        rc = ngx_http_send_header(r);

        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
            return rc;
        }
    }
   
    size = sizeof("Cache statistics:\n")
	    + sizeof("Requests: \n")	+ NGX_ATOMIC_T_LEN
	    + sizeof("Uncached: \n")    + NGX_ATOMIC_T_LEN
	    + sizeof("Miss: \n")	+ NGX_ATOMIC_T_LEN
	    + sizeof("Bypass: \n")	+ NGX_ATOMIC_T_LEN
	    + sizeof("Expired: \n")	+ NGX_ATOMIC_T_LEN
	    + sizeof("Stale: \n")	+ NGX_ATOMIC_T_LEN
	    + sizeof("Updating: \n")	+ NGX_ATOMIC_T_LEN
	    + sizeof("Revalidated: \n") + NGX_ATOMIC_T_LEN
	    + sizeof("Hit: \n")		+ NGX_ATOMIC_T_LEN
	    + sizeof("Misc: ")		+ NGX_ATOMIC_T_LEN;

    b = ngx_create_temp_buf(r->pool, size);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    out.buf = b;
    out.next = NULL;
    
    b->last = ngx_cpymem(b->last, "Cache statistics:\n", 
				  sizeof("Cache statistics:\n") - 1);
    b->last = ngx_sprintf(b->last, "Requests: %uA\n",       cache_status[0]);
    b->last = ngx_sprintf(b->last, "Uncached: %uA\n",       cache_status[1]);
    b->last = ngx_sprintf(b->last, "Miss: %uA\n",	    cache_status[2]);
    b->last = ngx_sprintf(b->last, "Bypass: %uA\n",	    cache_status[3]);
    b->last = ngx_sprintf(b->last, "Expired: %uA\n",	    cache_status[4]);
    b->last = ngx_sprintf(b->last, "Stale: %uA\n",	    cache_status[5]);
    b->last = ngx_sprintf(b->last, "Updating: %uA\n",	    cache_status[6]);
    b->last = ngx_sprintf(b->last, "Revalidated: %uA\n",    cache_status[7]);
    b->last = ngx_sprintf(b->last, "Hit: %uA\n",	    cache_status[8]);
    b->last = ngx_sprintf(b->last, "Misc: %uA",		    cache_status[9]);

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = b->last - b->pos;

    b->last_buf = (r == r->main) ? 1 : 0;
    b->last_in_chain = 1;

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    return ngx_http_output_filter(r, &out);
}

static char *ngx_set_status(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_status_handler;

    return NGX_CONF_OK;
}

static ngx_int_t ngx_cache_status_filter_init(ngx_conf_t *cf)
{   
    ngx_original_filter_ptr = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_cache_status_filter;
    
    return NGX_OK;
}

static ngx_int_t ngx_cache_status_filter(ngx_http_request_t *r) 
{
    cache_status[TOTAL_REQUESTS_SLOT]++;
#if (NGX_HTTP_CACHE)
    if (r->upstream == NULL || r->upstream->cache_status == 0) {
	cache_status[UNCACHED_REQUESTS_SLOT]++;
        return ngx_original_filter_ptr(r);
    }
    
    if (r->upstream->cache_status > 7) { //greater than NGX_HTTP_CACHE_HIT
        cache_status[MISC_SLOT]++;
    } else {
        cache_status[r->upstream->cache_status+1]++;
    }
#else
    cache_status[UNCACHED_REQUESTS_SLOT]++;
#endif
    
    return ngx_original_filter_ptr(r);
}
