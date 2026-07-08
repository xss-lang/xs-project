/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/diagnostic.h"
#include "xs/syntax_ast.h"
#include "xs/syntax_parser.h"

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

static size_t count_kind(const XsSyntaxNode *node, XsSyntaxKind kind)
{
  if (node == NULL)
    return 0;
  size_t count = node->kind == kind ? 1 : 0;
  for (size_t index = 0; index < node->child_count; ++index)
    count += count_kind(node->children[index], kind);
  return count;
}

static void test_top_level_variable_declaration_structure(void)
{
  const char *text = "data User { name: Str }\n"
                     "user: User = { set.name{\"Alfa\"}; };\n"
                     "public static Pi: Float = 3.141592653589793;\n";
  XsSource source = {.path = "TopLevelData.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 19, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_DECL_VARIABLE) == 2);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_EXPR_OBJECT_LITERAL) != NULL);
  CHECK(xs_syntax_find_first(tree.root, XS_SYNTAX_OBJECT_FIELD) != NULL);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_data_rejects_non_field_members(void)
{
  const char *texts[] = {
      "data User { fn GetName() {} }\n",      "data User { User(name: Str) {} }\n", "data User { User.Drop() {} }\n",
      "data User { implements Runnable; }\n", "data User { extends Person; }\n",
  };
  for (size_t index = 0; index < sizeof(texts) / sizeof(texts[0]); ++index)
  {
    XsSource source = {.path = "InvalidData.xs", .text = texts[index], .length = strlen(texts[index])};
    XsDiagnostics diagnostics;
    XsSyntaxTree tree;
    xs_diagnostics_init(&diagnostics);
    CHECK(!xs_syntax_parse(&source, 26 + index, &diagnostics, &tree));
    CHECK(xs_diagnostics_has_error(&diagnostics));
    xs_syntax_tree_free(&tree);
    xs_diagnostics_free(&diagnostics);
  }
}

static void test_class_constructor_rules(void)
{
  const char *valid = "class User { User(name: Str) {} }\n";
  XsSource source = {.path = "ConstructorValid.xs", .text = valid, .length = strlen(valid)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 31, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_CLASS_CONSTRUCTOR) == 1);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *invalid_name = "class User { Person() {} }\n";
  source = (XsSource){.path = "ConstructorNameInvalid.xs", .text = invalid_name, .length = strlen(invalid_name)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 32, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *duplicate = "class User { User() {} User(name: Str) {} }\n";
  source = (XsSource){.path = "ConstructorDuplicate.xs", .text = duplicate, .length = strlen(duplicate)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 33, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_interface_member_rules(void)
{
  const char *valid = "interface Runnable { fn Run(); fn Close(); }\n";
  XsSource source = {.path = "InterfaceValid.xs", .text = valid, .length = strlen(valid)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 34, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_DECL_FUNCTION) == 2);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *field = "interface Runnable { name: Str; }\n";
  source = (XsSource){.path = "InterfaceFieldInvalid.xs", .text = field, .length = strlen(field)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 35, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *body = "interface Runnable { fn Run() {} }\n";
  source = (XsSource){.path = "InterfaceBodyInvalid.xs", .text = body, .length = strlen(body)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 36, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_class_inheritance_rules(void)
{
  const char *valid = "class Animal {}\n"
                      "interface Runnable { fn Run(); }\n"
                      "interface Closeable { fn Close(); }\n"
                      "class Program { extends Animal; implements Runnable, Closeable; }\n";
  XsSource source = {.path = "ClassInheritanceValid.xs", .text = valid, .length = strlen(valid)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 45, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_TYPE_NAMED) == 3);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *interface_extends = "interface Runnable { extends Base; fn Run(); }\n";
  source =
      (XsSource){.path = "InterfaceExtendsInvalid.xs", .text = interface_extends, .length = strlen(interface_extends)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 46, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *multiple_extends = "class Program { extends Animal, Machine; }\n";
  source =
      (XsSource){.path = "MultipleExtendsInvalid.xs", .text = multiple_extends, .length = strlen(multiple_extends)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 47, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *duplicate_extends = "class Program { extends Animal; extends Machine; }\n";
  source =
      (XsSource){.path = "DuplicateExtendsInvalid.xs", .text = duplicate_extends, .length = strlen(duplicate_extends)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 48, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_incomplete_function_rules(void)
{
  const char *valid = "incomplete class Animal { incomplete fn Speak(); }\n";
  XsSource source = {.path = "IncompleteValid.xs", .text = valid, .length = strlen(valid)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 37, &diagnostics, &tree));
  const XsSyntaxNode *function = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_FUNCTION);
  CHECK(function != NULL);
  CHECK(function == NULL || (function->flags & XS_SYNTAX_FLAG_INCOMPLETE) != 0);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *missing_modifier = "class Animal { fn Speak(); }\n";
  source =
      (XsSource){.path = "IncompleteMissingModifier.xs", .text = missing_modifier, .length = strlen(missing_modifier)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 38, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *body = "class Animal { incomplete fn Speak() {} }\n";
  source = (XsSource){.path = "IncompleteBodyInvalid.xs", .text = body, .length = strlen(body)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 39, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_enum_payload_rules(void)
{
  const char *valid = "enum data Token { Identifier: Str, Integer: Int, Plus, }\n";
  XsSource source = {.path = "DataEnumValid.xs", .text = valid, .length = strlen(valid)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 40, &diagnostics, &tree));
  const XsSyntaxNode *declaration = xs_syntax_find_first(tree.root, XS_SYNTAX_DECL_ENUM);
  CHECK(declaration != NULL);
  CHECK(declaration == NULL || (declaration->flags & XS_SYNTAX_FLAG_DATA_ENUM) != 0);
  CHECK(count_kind(tree.root, XS_SYNTAX_ENUM_VARIANT) == 3);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *typed_regular = "enum Color { Red: Int, }\n";
  source = (XsSource){.path = "RegularEnumPayloadInvalid.xs", .text = typed_regular, .length = strlen(typed_regular)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 41, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *empty_data = "enum data Empty { }\n";
  source = (XsSource){.path = "DataEnumEmptyInvalid.xs", .text = empty_data, .length = strlen(empty_data)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 42, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);

  const char *tuple_payload = "enum data Token { Position: (Int, Int), }\n";
  source = (XsSource){.path = "DataEnumTupleInvalid.xs", .text = tuple_payload, .length = strlen(tuple_payload)};
  xs_diagnostics_init(&diagnostics);
  CHECK(!xs_syntax_parse(&source, 43, &diagnostics, &tree));
  CHECK(xs_diagnostics_has_error(&diagnostics));
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_generic_constraint_structure(void)
{
  const char *text = "interface Runnable { fn Run(); }\n"
                     "interface Printable { fn Print(); }\n"
                     "fn Execute<T: Runnable, Printable, U: Runnable>(value: T, worker: U) {}\n";
  XsSource source = {.path = "GenericConstraints.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 44, &diagnostics, &tree));
  CHECK(count_kind(tree.root, XS_SYNTAX_GENERIC_PARAMETER) == 2);
  CHECK(count_kind(tree.root, XS_SYNTAX_TYPE_NAMED) >= 4);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

int main(void)
{
  test_top_level_variable_declaration_structure();
  test_data_rejects_non_field_members();
  test_class_constructor_rules();
  test_interface_member_rules();
  test_class_inheritance_rules();
  test_incomplete_function_rules();
  test_enum_payload_rules();
  test_generic_constraint_structure();
  return failures == 0 ? 0 : 1;
}
