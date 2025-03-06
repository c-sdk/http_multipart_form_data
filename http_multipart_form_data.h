#ifndef __AIL_HTTP_MULTIPART_FORM_DATA_H__
#define __AIL_HTTP_MULTIPART_FORM_DATA_H__ 1

#ifdef LOG_HTTP_MULTIPART_FORM_DATA
#define http_multipart_form_data_log(...) printf(__VA_ARGS__)
#else
#define http_multipart_form_data_log(...)
#endif

#include "arena.h"
#include "list.h"
#include "string_map.h"

struct http_multipart_form_data_part_t {
  char* content;
  struct string_map_t headers;
};

struct http_multipart_form_data_t {
  char *mime_type;
  struct string_map_t attributes;
  struct array_t parts;
};

#define HTTP_MULTIPART_FORM_DATA_DEFAULT_ATTRIBUTES_SIZE 4

void http_multipart_form_data_init(arena_t* arena,
                                   struct http_multipart_form_data_t* formdata);

int http_multipart_form_data_parse_content_type_header(arena_t *arena,
                                                       struct http_multipart_form_data_t *data,
                                                       const char *text);

int http_multipart_form_data_parse_content(arena_t* arena,
                                           struct array_t* parts,
                                           const char* const boundary,
                                           const char* text);

int http_multipart_form_data_parse(arena_t* arena,
                                   struct string_map_t* request_headers,
                                   const char* const request_content,
                                   struct http_multipart_form_data_t* formdata);

#endif // __AIL_HTTP_MULTIPART_FORM_DATA_H__
