#include "xs/lil.h"
#include "xs/lil/aot.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                                                               \
  do                                                                                                                   \
  {                                                                                                                    \
    if (!(condition))                                                                                                  \
    {                                                                                                                  \
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
  if (stream == NULL)
  {
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

static void test_function_body_text_writer(void)
{
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_create("App.Body", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = NULL;
  CHECK(xs_lil_module_add_function_definition(module, "Answer", (XsLilType){.kind = XS_LIL_TYPE_I64}, NULL, 0,
                                              &function, &error) == XS_LIL_OK);
  CHECK(function != NULL);
  XsLilBlock *entry = NULL;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  XsLilValueId value = 0;
  CHECK(xs_lil_block_add_const_i64(entry, 42, &value, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return_value(entry, value, &error) == XS_LIL_OK);

  FILE *stream = tmpfile();
  if (stream == NULL)
  {
    ++failures;
    xs_lil_module_destroy(module);
    return;
  }
  CHECK(xs_lil_module_write_text(module, stream, &error) == XS_LIL_OK);
  CHECK(fseek(stream, 0, SEEK_SET) == 0);
  char buffer[256] = {0};
  size_t read = fread(buffer, 1, sizeof(buffer) - 1, stream);
  buffer[read] = '\0';
  CHECK(strstr(buffer, "define i64 Answer() {\n") != NULL);
  CHECK(strstr(buffer, "bb0 entry:\n  r0 = const.i64 42\n  return r0\n}\n") != NULL);
  fclose(stream);
  xs_lil_module_destroy(module);
}

static void test_function_body_rejects_missing_return_value(void)
{
  XsLilError error = {0};
  XsLilModule *module = NULL;
  CHECK(xs_lil_module_create("App.BadBody", &module, &error) == XS_LIL_OK);
  XsLilFunction *function = NULL;
  CHECK(xs_lil_module_add_function_definition(module, "Bad", (XsLilType){.kind = XS_LIL_TYPE_I64}, NULL, 0, &function,
                                              &error) == XS_LIL_OK);
  XsLilBlock *entry = NULL;
  CHECK(xs_lil_function_append_block(function, "entry", &entry, &error) == XS_LIL_OK);
  CHECK(xs_lil_block_set_return(entry, &error) == XS_LIL_INVALID_ARGUMENT);
  xs_lil_module_destroy(module);
}

int main(void)
{
  test_module_and_text_writer();
  test_function_body_text_writer();
  test_function_body_rejects_missing_return_value();
  return failures == 0 ? 0 : 1;
}
