#ifndef XS_PROJECT_H
#define XS_PROJECT_H

#include "xs/diagnostic.h"

#include <stddef.h>

typedef struct
{
  bool is_nil;
  char *text;
  XsSpan span;
} XsProjectValue;

typedef struct
{
  XsProjectValue name;
  XsProjectValue email;
} XsProjectAuthor;

typedef struct
{
  XsProjectValue os_name;
  XsProjectValue os_arch;
} XsProjectTarget;

typedef struct
{
  XsProjectValue name;
  XsProjectValue version;
} XsProjectModule;

typedef struct
{
  XsProjectValue app_name;
  XsProjectValue app_version;
  XsProjectValue app_release;
  XsProjectValue app_license;
  XsProjectAuthor *authors;
  size_t author_count;
  XsProjectValue xs_version;
  XsProjectValue xs_backend;
  XsProjectValue entry;
  XsProjectValue *additional_files;
  size_t additional_file_count;
  XsProjectTarget *targets;
  size_t target_count;
  XsProjectModule *external_modules;
  size_t external_module_count;
} XsProject;

void xs_project_init(XsProject *project);
void xs_project_free(XsProject *project);
bool xs_project_parse(const XsSource *source, XsDiagnostics *diagnostics, XsProject *project);
const XsProjectValue *xs_project_selected_entry(const XsProject *project);

#endif
