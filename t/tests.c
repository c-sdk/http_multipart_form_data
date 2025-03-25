#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "arena.h"
#include "http_multipart_form_data.h"
#include "status.h"
#include "string_map.h"


void test_parse_a_valid_content_type() {
  arena_t arena = {0};
  arena_create(&arena, getpagesize());

  const char *header = "multipart/form-data; boundary=----WebKitFormBoundaryXYZ";
  struct http_multipart_form_data_t result = {0};
  http_multipart_form_data_init(&arena, &result);

  int res = http_multipart_form_data_parse_content_type_header(&arena, &result, header);

  assert(res == 0);
  assert(result.mime_type && strcmp(result.mime_type, "multipart/form-data") == 0);
  struct string_map_entry_t *found_item = NULL;
  char* value = string_map_find_by_key(&result.attributes, &found_item, "boundary");
  assert(value && strcmp(value, "----WebKitFormBoundaryXYZ") == 0);

  arena_free(&arena);
}

void test_fail_to_parse_when_missing_boundary() {
  arena_t arena = {0};
  arena_create(&arena, getpagesize());

  const char *header = "multipart/form-data";
  struct http_multipart_form_data_t result = {0};
  http_multipart_form_data_init(&arena, &result);

  int res = http_multipart_form_data_parse_content_type_header(&arena, &result, header);

  assert(res == -1);
  assert(result.mime_type == NULL);
  assert(result.attributes.count == 0);

  arena_free(&arena);
}

void test_parse_multipart_content() {
  arena_t arena = {0};
  arena_create(&arena, getpagesize());

  const char *boundary = "----WebKitFormBoundaryXYZ";
  const char *request_body =
    "------WebKitFormBoundaryXYZ\r\n"
    "Content-Disposition: form-data; name=\"field1\"\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    "value1\r\n"
    "------WebKitFormBoundaryXYZ\r\n"
    "Content-Disposition: form-data; name=\"field2\"\r\n"
    "Content-Type: application/json\r\n"
    "\r\n"
    "{\"key\": \"value\"}\r\n"
    "------WebKitFormBoundaryXYZ--\r\n";

  struct http_multipart_form_data_t result = {0};
  http_multipart_form_data_init(&arena, &result);
  enum status_t res = http_multipart_form_data_parse_content(&arena,
                                                             &result.parts,
                                                             boundary,
                                                             request_body);

  assert(status_is_ok(res));
  struct http_multipart_form_data_part_t* item = NULL;

  {
    item = result.parts.data[0];
    assert(memcmp(item->content, "value1", strlen("value1")) == 0);
    char
      *content_disposition_key = "Content-Disposition",
      *content_disposition_value = "form-data; name=\"field1\"";
    assert(memcmp(item->headers.data[0].key, content_disposition_key,
                  strlen(content_disposition_key)) == 0);
    assert(memcmp(item->headers.data[0].value, content_disposition_value,
                  strlen(content_disposition_value)) == 0);
    char
      *content_type_key = "Content-Type",
      *content_type_value = "text/plain";
    assert(memcmp(item->headers.data[1].key, content_type_key,
                  strlen(content_type_key)) == 0);
    assert(memcmp(item->headers.data[1].value, content_type_value,
                  strlen(content_type_value)) == 0);
  }

  {
    item = result.parts.data[0];
    char* content = "value1";
    assert(memcmp(item->content, content, strlen(content)) == 0);
  }

  {
    item = result.parts.data[1];
    char* content = "{\"key\": \"value\"}";
    assert(memcmp(item->content, content, strlen(content)) == 0);
  }
}

int main(void) {
  printf("test http multipart form data\n");
  test_parse_a_valid_content_type();
  test_fail_to_parse_when_missing_boundary();
  test_parse_multipart_content();
  printf("done.\n");
  return 0;
}
