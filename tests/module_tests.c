/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/hir/module_registry.h"

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

static void test_discovery_and_resolution(const char *root)
{
  XsModuleRegistry registry;
  XsModuleGraph graph;
  XsModuleIssues issues;
  xs_module_registry_init(&registry);
  xs_module_graph_init(&graph);
  xs_module_issues_init(&issues);
  CHECK(xs_module_registry_discover(root, &registry, &issues));
  CHECK(registry.count == 1);
  const XsDiscoveredModule *module = xs_module_registry_find(&registry, "Math.Advanced");
  CHECK(module != NULL);
  const char *direct[] = {"source/Main.xs"};
  CHECK(xs_module_graph_resolve(root, direct, 1, &registry, &graph, &issues));
  CHECK(graph.count == 1);
  CHECK(strcmp(graph.dependencies[0].module_name, "Math.Advanced") == 0);
  xs_module_issues_free(&issues);
  xs_module_graph_free(&graph);
  xs_module_registry_free(&registry);
}

static void test_duplicate_modules(const char *root)
{
  XsModuleRegistry registry;
  XsModuleIssues issues;
  xs_module_registry_init(&registry);
  xs_module_issues_init(&issues);
  CHECK(!xs_module_registry_discover(root, &registry, &issues));
  CHECK(issues.count == 1);
  xs_module_issues_free(&issues);
  xs_module_registry_free(&registry);
}

int main(int argc, char **argv)
{
  if(argc != 3)
    return 2;
  test_discovery_and_resolution(argv[1]);
  test_duplicate_modules(argv[2]);
  return failures == 0 ? 0 : 1;
}
