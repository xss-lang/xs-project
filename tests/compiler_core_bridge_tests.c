/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "xs/compiler_core.h"
#include "xs/diagnostic.h"
#include "xs/macro.h"
#include "xs/syntax_parser.h"

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

static bool record_text_equals(const XsCompilerCoreSyntaxPacket *packet, const XsCompilerCoreSyntaxNode *node,
                               const char *expected)
{
  size_t length = strlen(expected);
  return node->text_length == length && memcmp(packet->text_bytes + node->text_offset, expected, length) == 0;
}

static void test_materialized_syntax_packet(void)
{
  const char *text = "module app;\n"
                     "macro_rules! make { () -> { incomplete fn generated(); }; }\n"
                     "make!();\n"
                     "fn main() -> Long { value: Long = 3; value = 7; return value; }\n";
  XsSource source = {.path = "Bridge.xs", .text = text, .length = strlen(text)};
  XsDiagnostics diagnostics;
  XsSyntaxTree tree;
  XsSyntaxTree expanded;
  XsMacroDeclarationExpansionSet declarations;
  XsMacroStatementExpansionSet statements;
  XsCompilerCoreSyntaxStorage *storage = nullptr;
  xs_diagnostics_init(&diagnostics);
  CHECK(xs_syntax_parse(&source, 73, &diagnostics, &tree));
  CHECK(xs_macro_validate(&tree, &diagnostics));
  CHECK(xs_macro_expand_declarations(&tree, &diagnostics, &declarations));
  CHECK(xs_macro_expand_statements(&tree, &diagnostics, &statements));
  CHECK(xs_macro_materialize_expanded_tree(&tree, &declarations, &statements, &diagnostics, &expanded));
  CHECK(xs_compiler_core_syntax_packet_create(&expanded, &storage) == XS_COMPILER_CORE_OK);
  const XsCompilerCoreSyntaxPacket *packet = xs_compiler_core_syntax_packet(storage);
  CHECK(packet != nullptr);
  CHECK(packet == nullptr || packet->abi_version == 0);
  CHECK(packet == nullptr || packet->node_count != 0);
  CHECK(packet == nullptr || packet->root_index < packet->node_count);
  XsCompilerCoreSession *session = nullptr;
  CHECK(packet == nullptr || xslang_compiler_core_session_create(packet, &session) == XS_COMPILER_CORE_FFI_OK);
  CHECK(packet == nullptr || xslang_compiler_core_session_syntax_node_count(session) == packet->node_count);
  CHECK(packet == nullptr || xslang_compiler_core_session_function_count(session) == 2);
  CHECK(packet == nullptr || xslang_compiler_core_session_mir_function_count(session) == 1);
  if(packet != nullptr)
  {
    const XsCompilerCoreSyntaxNode *root = &packet->nodes[packet->root_index];
    CHECK(root->kind == XS_SYNTAX_FILE);
    CHECK(root->parent_index == XS_COMPILER_CORE_NO_NODE);
    CHECK(root->child_count != 0);
    bool found_generated = false;
    for(uint64_t index = 0; index < packet->node_count; ++index)
    {
      const XsCompilerCoreSyntaxNode *node = &packet->nodes[index];
      CHECK(node->first_child + node->child_count <= packet->child_index_count);
      CHECK(node->text_offset + node->text_length <= packet->text_byte_count);
      if(node->kind == XS_SYNTAX_IDENTIFIER && record_text_equals(packet, node, "generated"))
        found_generated = true;
      for(uint64_t child = 0; child < node->child_count; ++child)
      {
        uint64_t child_index = packet->child_indices[node->first_child + child];
        CHECK(child_index < packet->node_count);
        CHECK(child_index >= packet->node_count || packet->nodes[child_index].parent_index == index);
      }
    }
    CHECK(found_generated);
  }
  xslang_compiler_core_session_free(session);
  xs_compiler_core_syntax_packet_free(storage);
  xs_syntax_tree_free(&expanded);
  xs_macro_statement_expansion_set_free(&statements);
  xs_macro_declaration_expansion_set_free(&declarations);
  xs_syntax_tree_free(&tree);
  xs_diagnostics_free(&diagnostics);
}

static void test_invalid_packet_inputs(void)
{
  XsCompilerCoreSyntaxStorage *storage = nullptr;
  CHECK(xs_compiler_core_syntax_packet_create(nullptr, &storage) == XS_COMPILER_CORE_INVALID_ARGUMENT);
  CHECK(xs_compiler_core_syntax_packet_create(&(XsSyntaxTree){0}, &storage) == XS_COMPILER_CORE_INVALID_ARGUMENT);
  CHECK(xs_compiler_core_syntax_packet(nullptr) == nullptr);
  XsCompilerCoreSession *session = nullptr;
  CHECK(xslang_compiler_core_session_create(nullptr, &session) == XS_COMPILER_CORE_FFI_NULL_ARGUMENT);
  CHECK(xslang_compiler_core_session_create(nullptr, nullptr) == XS_COMPILER_CORE_FFI_NULL_ARGUMENT);
  XsCompilerCoreSyntaxPacket invalid = {.abi_version = XS_COMPILER_CORE_SYNTAX_ABI_VERSION + 1U};
  CHECK(xslang_compiler_core_session_create(&invalid, &session) == XS_COMPILER_CORE_FFI_INVALID_PACKET);
  xs_compiler_core_syntax_packet_free(nullptr);
}

int main(void)
{
  test_materialized_syntax_packet();
  test_invalid_packet_inputs();
  return failures == 0 ? 0 : 1;
}
