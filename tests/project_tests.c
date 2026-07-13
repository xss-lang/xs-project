/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/diagnostic.h"
#include "xs/project.h"

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                                                               \
  do                                                                                                                   \
  {                                                                                                                    \
    if(!(condition))                                                                                                   \
    {                                                                                                                  \
      fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                                    \
      ++failures;                                                                                                      \
    }                                                                                                                  \
  } while(0)

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
                     "appVersion: \"0.0.1\"\n"
                     "appRelease: \"BETA\"\n"
                     "appLicense: \"MIT\"\n"
                     "appAuthors {\n[\"Alpha\", \"alfa@example.me\"]\n[\"Foo\", None]\n}\n"
                     "compilerOptions {\n"
                     "xsVersion: \"26\"\n"
                     "addFiles {\nentry: \"source/Main.xs\"\n[\"source/Foo.xs\", \"source/Bar.xs\",]\n}\n"
                     "output {\n[osName: \"Linux\"; osArch: \"x64\"]\n}\n"
                     "}\n"
                     "externalModules {\n"
                     "addModule { moduleName: \"xsmods.JSON\"; moduleRepo: "
                     "\"https://github.com/xss-lang/externalModules\"; moduleVersion: \"0.1\" }\n"
                     "}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(parse_project(text, &project, &diagnostics));
  CHECK(project.author_count == 2);
  CHECK(project.additional_file_count == 2);
  CHECK(project.target_count == 1);
  CHECK(project.external_module_count == 1);
  CHECK(xs_project_external_module_count(&project) == 1);
  const XsProjectModule *module = xs_project_external_module_at(&project, 0);
  CHECK(module == &project.external_modules[0]);
  CHECK(xs_project_external_module_at(&project, 1) == nullptr);
  CHECK(xs_project_external_module_count(nullptr) == 0);
  CHECK(xs_project_external_module_name(module) == &module->name);
  CHECK(xs_project_external_module_repo(module) == &module->repo);
  CHECK(xs_project_external_module_version(module) == &module->version);
  CHECK(xs_project_external_module_repo(nullptr) == nullptr);
  const XsProjectValue *repo = xs_project_external_module_repo(module);
  CHECK(repo != nullptr && repo->text != nullptr &&
        strcmp(repo->text, "https://github.com/xss-lang/externalModules") == 0);
  CHECK(xs_project_selected_entry(&project) == &project.entry);
  CHECK(xs_project_selected_entry(nullptr) == nullptr);
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_none_entry_selects_first_file(void)
{
  const char *text = "appName: None\nappVersion: None\nappRelease: None\nappLicense: None\n"
                     "appAuthors { [None, None] }\n"
                     "compilerOptions {\nxsVersion: None\n"
                     "addFiles { entry: None; [\"source/Foo.xs\"] }\n"
                     "output { [osName: None; osArch: None] }\n}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(parse_project(text, &project, &diagnostics));
  const XsProjectValue *entry = xs_project_selected_entry(&project);
  CHECK(entry != nullptr && entry->text != nullptr && strcmp(entry->text, "source/Foo.xs") == 0);
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_invalid_manifest(void)
{
  const char *text = "appName: \"A\" appVersion: \"1\"\n"
                     "appRelease: \"RC\"\nappLicense: None\nappAuthors {}\ncompilerOptions {}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(!parse_project(text, &project, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_optimization_is_unknown(void)
{
  const char *text = "appName: None\nappVersion: None\nappRelease: None\nappLicense: None\n"
                     "appAuthors { [None, None] }\n"
                     "compilerOptions {\n"
                     "xsVersion: \"26\"; optimization: \"release\"\n"
                     "addFiles { entry: None }\n"
                     "output { [osName: None; osArch: None] }\n"
                     "}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(!parse_project(text, &project, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_removed_backend_option_is_unknown(void)
{
  const char *text = "appName: None\nappVersion: None\nappRelease: None\nappLicense: None\n"
                     "appAuthors { [None, None] }\n"
                     "compilerOptions {\n"
                     "xsVersion: \"26\"; xsBackend: \"LLVM\"\n"
                     "addFiles { entry: None }\n"
                     "output { [osName: None; osArch: None] }\n"
                     "}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(!parse_project(text, &project, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_external_module_repo_is_required(void)
{
  const char *text = "appName: None\nappVersion: None\nappRelease: None\nappLicense: None\n"
                     "appAuthors { [None, None] }\n"
                     "compilerOptions {\n"
                     "xsVersion: None\n"
                     "addFiles { entry: None }\n"
                     "output { [osName: None; osArch: None] }\n"
                     "}\n"
                     "externalModules {\n"
                     "addModule { moduleName: \"xsmods.JSON\"; moduleVersion: \"0.1\" }\n"
                     "}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(!parse_project(text, &project, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

static void test_project_comments_are_line_only(void)
{
  const char *valid = "/// project doc comment\n"
                      "appName: None // line comment\n"
                      "appVersion: None\n"
                      "appRelease: None\n"
                      "appLicense: None\n"
                      "appAuthors { [None, None] }\n"
                      "compilerOptions {\n"
                      "xsVersion: None\n"
                      "addFiles { entry: None }\n"
                      "output { [osName: None; osArch: None] }\n"
                      "}\n";
  XsProject project;
  XsDiagnostics diagnostics;
  CHECK(parse_project(valid, &project, &diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);

  const char *xs_style_multiline = "//{ not supported in xsproj\n";
  CHECK(!parse_project(xs_style_multiline, &project, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);

  const char *c_style_multiline = "/* not supported */\nappName: None\n";
  CHECK(!parse_project(c_style_multiline, &project, &diagnostics));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_project_free(&project);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_complete_project();
  test_none_entry_selects_first_file();
  test_invalid_manifest();
  test_optimization_is_unknown();
  test_removed_backend_option_is_unknown();
  test_external_module_repo_is_required();
  test_project_comments_are_line_only();
  return failures == 0 ? 0 : 1;
}
