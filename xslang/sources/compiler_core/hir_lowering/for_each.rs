/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

pub(super) fn lower_for_each_statement(tree: &SyntaxTree,
                                       statement: &SyntaxNode,
                                       context: &LoweringContext,
                                       locals: &HashMap<String, Type>,
                                       return_type: Option<&Type>)
                                       -> Option<Statement>
{
  let pattern = statement.children.first().and_then(|index| tree.nodes.get(*index))?;
  let (identifier_pattern, declared_type) = if pattern.kind == PATTERN_TYPED
  {
    let inner = pattern.children.first().and_then(|index| tree.nodes.get(*index))?;
    let ty = pattern.children.get(1).and_then(|index| tree.nodes.get(*index))?;
    (inner, Some(checked_type(&lower_type(tree, ty))?))
  }
  else
  {
    (pattern, None)
  };
  if identifier_pattern.kind != PATTERN_IDENTIFIER
  {
    return None;
  }
  let name = first_child_kind(tree, identifier_pattern, IDENTIFIER)?.text.clone();
  let iterable_node = statement.children.get(1).and_then(|index| tree.nodes.get(*index))?;
  let iterable_type = expression_type(tree, iterable_node, context, locals)?;
  let inferred_binding_type = match &iterable_type
  {
    Type::Array { element, .. } => Some(element.as_ref().clone()),
    _ => None,
  };
  let binding_type = declared_type.or(inferred_binding_type)?;
  let iterable = lower_expression(tree, iterable_node, context, locals, Some(&iterable_type))?;
  let body_node = statement.children.last().and_then(|index| tree.nodes.get(*index))?;
  let mut body_locals = locals.clone();
  body_locals.insert(name.clone(), binding_type.clone());
  let body = lower_hir_block(tree, body_node, context, &mut body_locals, return_type, None)?;
  Some(Statement::ForEach { binding: crate::hir::type_check::Local { name,
                                                                     ty: binding_type,
                                                                     mutable: false,
                                                                     span: span(pattern)? },
                            iterable,
                            iterable_type,
                            body,
                            span: span(statement)? })
}
