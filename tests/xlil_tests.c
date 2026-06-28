#include "xs/lil.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                                                               \
  do {                                                                                                                 \
    if (!(condition)) {                                                                                                \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                                    \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while (0)

static void test_module_and_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_create("App.Main", &module, &error) == XS_LIL_OK);
  CHECK(module != NULL);
  CHECK(strcmp(xs_lil_module_name(module), "App.Main") == 0);
  const XsLilType parameters[] = {
      {.kind = XS_LIL_TYPE_U8},
      {.kind = XS_LIL_TYPE_I8},
      {.kind = XS_LIL_TYPE_BOOL},
  };
  CHECK(xs_lil_module_add_function(module, "Check", (XsLilType){.kind = XS_LIL_TYPE_BOOL}, parameters, 3, &error) ==
        XS_LIL_OK);
  CHECK(xs_lil_module_function_count(module) == 1);
  CHECK(strcmp(xs_lil_type_name((XsLilType){.kind = XS_LIL_TYPE_BOOL}), "bool") == 0);
  CHECK(strcmp(xs_lil_type_name((XsLilType){.kind = XS_LIL_TYPE_U8}), "u8") == 0);
  CHECK(strcmp(xs_lil_type_name((XsLilType){.kind = XS_LIL_TYPE_I8}), "i8") == 0);

  FILE *stream = tmpfile();
  if (stream == NULL) {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[128] = {0};
  CHECK(fgets(buffer, sizeof(buffer), stream) != NULL);
  CHECK(strstr(buffer, "xlil module App.Main\n") != NULL);
  CHECK(fgets(buffer, sizeof(buffer), stream) != NULL);
  CHECK(strstr(buffer, "declare bool Check(u8, i8, bool)\n") != NULL);
  fclose(stream);
  xs_lil_module_destroy(module);
}

int main(void)
{
  test_module_and_text_writer();
  return failures == 0 ? 0 : 1;
}
