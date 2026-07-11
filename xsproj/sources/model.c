/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/project.h"

#include <stdlib.h>

void xs_project_init(XsProject *project)
{
  *project = (XsProject){0};
}

static void free_value(XsProjectValue *value)
{
  free(value->text);
  *value = (XsProjectValue){0};
}

void xs_project_free(XsProject *project)
{
  free_value(&project->app_name);
  free_value(&project->app_version);
  free_value(&project->app_release);
  free_value(&project->app_license);
  for (size_t i = 0; i < project->author_count; ++i)
  {
    free_value(&project->authors[i].name);
    free_value(&project->authors[i].email);
  }
  free(project->authors);
  free_value(&project->xs_version);
  free_value(&project->xs_backend);
  free_value(&project->entry);
  for (size_t i = 0; i < project->additional_file_count; ++i)
    free_value(&project->additional_files[i]);
  free(project->additional_files);
  for (size_t i = 0; i < project->target_count; ++i)
  {
    free_value(&project->targets[i].os_name);
    free_value(&project->targets[i].os_arch);
  }
  free(project->targets);
  for (size_t i = 0; i < project->external_module_count; ++i)
  {
    free_value(&project->external_modules[i].name);
    free_value(&project->external_modules[i].repo);
    free_value(&project->external_modules[i].version);
  }
  free(project->external_modules);
  *project = (XsProject){0};
}

const XsProjectValue *xs_project_selected_entry(const XsProject *project)
{
  if (!project->entry.is_nil && project->entry.text != NULL)
    return &project->entry;
  if (project->additional_file_count != 0)
    return &project->additional_files[0];
  return NULL;
}
