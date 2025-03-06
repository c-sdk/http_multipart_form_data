#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "arena.h"
#include "array.h"
#include "string_map.h"
#include "strnstr.h"
#include "http_multipart_form_data.h"


void http_multipart_form_data_init(arena_t* arena, struct http_multipart_form_data_t* formdata) {
  string_map_init_with_arena(arena, &formdata->attributes, HTTP_MULTIPART_FORM_DATA_DEFAULT_ATTRIBUTES_SIZE);
  array_init(arena, &formdata->parts, HTTP_MULTIPART_FORM_DATA_DEFAULT_ATTRIBUTES_SIZE);
}


static void read_header_key_value_pair(arena_t* arena,
                                       char* position, char* next,
                                       char** key, char** value) {
  char* separator = strchr(position, '=');
  size_t key_length = separator - position + 1;
  size_t value_length = next - separator + 1;
  *key = arena_string_with_null(arena, position, key_length);
  *value = arena_string_with_null(arena, separator + 1, value_length);
}

// const char *header = "multipart/form-data; boundary=----WebKitFormBoundaryXYZ";


// TODO(dias): implement quoted boundary.
// const char *header = "multipart/form-data; boundary=\"WebKitFormBoundaryXYZ\"";
int http_multipart_form_data_parse_content_type_header(
  arena_t* arena,
  struct http_multipart_form_data_t* data,
  const char* text) {
  size_t length = strlen(text);
  assert(arena && data && text && length > 0);

  char *separator = strchr((char*)text, ';');

  // NOTE: if separator is not found, only the content-type exists
  // and it is illegal.
  if (separator == NULL) {
    return -1;
  }

  data->mime_type = arena_string_with_null(arena, text, separator - text + 1);

  // first ++ skip the ';' and walk
  while (*++separator != 0 && *separator == ' ');

  char* position = separator, *next = NULL;

  char* key = NULL, *value = NULL;

  while((next = strchr((char*)position, ';')) && next-- != NULL) {
    if (next == position) { break; }
    read_header_key_value_pair(arena,
                               next,
                               next + strlen(next) - 1,
                               &key, &value);
    string_map_add((struct string_map_t*)&data->attributes, key, value);
    position = (char*)next + 2;
  }

  if (position != NULL && position[0] != 0) {
    read_header_key_value_pair(arena,
                               position,
                               position + strlen(position),
                               &key, &value);
    string_map_add(&data->attributes, key, value);
  }

  struct string_map_entry_t *found_item = NULL;
  (void)string_map_find_by_key(&data->attributes, &found_item, "boundary");
  return (found_item != NULL) - 1;
}

struct http_multipart_form_data_part_t* http_multipart_form_data_create(arena_t* arena) {
  struct http_multipart_form_data_part_t* part =
    arena_alloc(arena, sizeof(struct http_multipart_form_data_part_t));
  // NOTE: The RFC 7578 says that it's expected only 3 headers
  // others can be ignored
  part->content = NULL;
  string_map_init_with_arena(arena, &part->headers, 3);

  return part;
}

static bool inline is_init_boundary(const char* position) {
  return position[0] == '-' && position[1] == '-';
}

static bool inline is_end_of_line(const char *position) {
  return position[0] == '\r' && position[1] == '\n';
}

enum hmfp_states_t {
  HTTP_MULTIPART_FORM_DATA_BOUNDARY_NAME, // text
  HTTP_MULTIPART_FORM_DATA_PART_HEADER_LINE,
  HTTP_MULTIPART_FORM_DATA_PART_CONTENT, // remainder
  HTTP_MULTIPART_FORM_DATA_PART_DONE // last -- after the boundary name
};

struct _parser_state_t {
  enum hmfp_states_t state;
};

int http_multipart_form_data_parse_content(arena_t *arena,
                                           struct array_t *parts,
                                           const char *const boundary,
                                           const char *text) {
  struct _parser_state_t parser = {0};

  char* position = (char*)text;

  struct http_multipart_form_data_part_t* part = NULL;

  while (parser.state != HTTP_MULTIPART_FORM_DATA_PART_DONE) {
    switch (parser.state) {
    case HTTP_MULTIPART_FORM_DATA_BOUNDARY_NAME: {
      if (is_init_boundary(position) == false) {
        return -1;
      }

      position += 2;

      int length = strlen(boundary);
      if (memcmp(position, boundary, length) != 0) {
        http_multipart_form_data_log("Error: Expected http multpart form data boundary.");
        return -1;
      }
      position += length;

      if (is_end_of_line(position)) {
        http_multipart_form_data_log("Found http multpart form data part.");
        part = http_multipart_form_data_create(arena);
        array_add(parts, part);
        parser.state = HTTP_MULTIPART_FORM_DATA_PART_HEADER_LINE;
      } else if (is_init_boundary(position) == true) {
        parser.state = HTTP_MULTIPART_FORM_DATA_PART_DONE;
      }

      position += 2;
    } break;
    case HTTP_MULTIPART_FORM_DATA_PART_HEADER_LINE: {
      // header end
      char* endline = strnstr(position, "\r\n", strlen(position));
      if (position == endline) {
        parser.state = HTTP_MULTIPART_FORM_DATA_PART_CONTENT;
        position += 2;
        continue;
      }
      char* separator = strchr(position, ':');
      if (separator == NULL) {
        http_multipart_form_data_log("Error: Expected header separator ':'");
        return -1;
      }
      const char* key = arena_string_with_null(arena, position, (separator - position + 1));
      while (*++separator != 0 && *separator == ' ');
      const char* value = arena_string_with_null(arena, separator, (endline - separator + 1));
      string_map_add(&part->headers, key, (void*)value);
      position = (char*)endline + 2;
    } break;
    case HTTP_MULTIPART_FORM_DATA_PART_CONTENT: {
      const char* const endline = strnstr(position, "\r\n", strlen(position));
      const char* const content = arena_string_with_null(arena, position, (endline - position + 1));
      part->content = (char*)content;
      parser.state = HTTP_MULTIPART_FORM_DATA_BOUNDARY_NAME;
      position = (char*)endline + 2;
    } break;
    default:;
    }
  }

  return 0;
}

int http_multipart_form_data_parse(arena_t* arena,
                                   struct string_map_t* request_headers,
                                   const char* const request_content,
                                   struct http_multipart_form_data_t* formdata) {
  char* content_type =
    string_map_find_by_key(request_headers,
                           NULL,
                           "Content-Type");

    int res = http_multipart_form_data_parse_content_type_header(arena,
                                                                 formdata,
                                                                 content_type);
    struct string_map_entry_t *found_item = NULL;
    char* boundary_name = string_map_find_by_key(&formdata->attributes,
                                                 &found_item,
                                                 "boundary");

    res = http_multipart_form_data_parse_content(arena,
                                                 &formdata->parts,
                                                 boundary_name,
                                                 request_content);
    return res;
}
