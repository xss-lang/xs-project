#include "xs/diagnostic.h"
#include "xs/project.h"

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

static bool parse_project(const char *text, XsProject *project, XsDiagnostics *diagnostics)
{
  XsSource source = {.path = "Test.xsproj", .text = text, .length = strlen(text)};
  xs_project_init(project);
  xs_diagnostics_init(diagnostics);
  return xs_project_parse(&source, diagnostics, project);
}

static void test_complete_project(void)
{
  const char *text = "appName: \"MyApp\"\n"
                     "appVersion: \"0.1.0\"\n"
                     "appRelease: \"BETA\"\n"
                     "appLicense: \"MIT\"\n"
                     "appAuthors {\n[\"Alfa\", \"alfa@example.me\"]\n[\"Foo\", nil]\n}\n"
                     "compilerOptions {\n"
                     "xsVersion: \"26\"\n"
                     "addFiles {\nentry: \"source/Main.xs\"\n[\"source/Foo.xs\", \"source/Bar.xs\",]\n}\n"
                     "output {\n[osName: \"Linux\"; osArch: \"x64\"]\n}\n"
                     "}\n"
                     "externalModules {\n"
                     "addModule { moduleName: \"xs.Json\"; moduleVersion: \"latest\" }\n"
                     "}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(parse_project(text, &project, &diagnostics));
  CHECK(project.author_count == 2);
  CHECK(project.additional_file_count == 2);
  CHECK(project.target_count == 1);
  CHECK(project.external_module_count == 1);
  CHECK(xs_project_selected_entry(&project) == &project.entry);
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_nil_entry_selects_first_file(void)
{
  const char *text = "appName: nil\nappVersion: nil\nappRelease: nil\nappLicense: nil\n"
                     "appAuthors { [nil, nil] }\n"
                     "compilerOptions {\nxsVersion: nil\n"
                     "addFiles { entry: nil; [\"source/Foo.xs\"] }\n"
                     "output { [osName: nil; osArch: nil] }\n}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(parse_project(text, &project, &diagnostics));
  const XsProjectValue *entry = xs_project_selected_entry(&project);
  CHECK(entry != NULL && entry->text != NULL && strcmp(entry->text, "source/Foo.xs") == 0);
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_invalid_manifest(void)
{
  const char *text = "appName: \"A\" appVersion: \"1\"\n"
                     "appRelease: \"RC\"\nappLicense: nil\nappAuthors {}\ncompilerOptions {}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(!parse_project(text, &project, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_optimization_is_unknown(void)
{
  const char *text = "appName: nil\nappVersion: nil\nappRelease: nil\nappLicense: nil\n"
                     "appAuthors { [nil, nil] }\n"
                     "compilerOptions {\n"
                     "xsVersion: \"26\"; optimization: \"release\"\n"
                     "addFiles { entry: nil }\n"
                     "output { [osName: nil; osArch: nil] }\n"
                     "}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(!parse_project(text, &project, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_complete_project();
  test_nil_entry_selects_first_file();
  test_invalid_manifest();
  test_optimization_is_unknown();
  return failures == 0 ? 0 : 1;
}
