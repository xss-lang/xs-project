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
  let iterable_node = statement.children.get(1).and_then(|index| tree.nodes.get(*index))?;
  let iterable_type = expression_type(tree, iterable_node, context, locals)?;
  let Type::Array { element, .. } = &iterable_type
  else
  {
    return None;
  };
  let (pattern, declared_type) = typed_pattern(tree, pattern)?;
  if declared_type.as_ref()
                  .is_some_and(|declared| declared != element.as_ref())
  {
    return None;
  }
  let binding_type = element.as_ref().clone();
  let iterable = lower_expression(tree, iterable_node, context, locals, Some(&iterable_type))?;
  let pattern_span = span(pattern)?;
  let (binding, mut body_locals, prefix) = lower_binding(tree,
                                                         pattern,
                                                         &binding_type,
                                                         locals,
                                                         pattern_span,
                                                         statement.span.start_offset)?;
  let body_node = statement.children.last().and_then(|index| tree.nodes.get(*index))?;
  let mut body = lower_hir_block(tree, body_node, context, &mut body_locals, return_type, None)?;
  body.statements.splice(0..0, prefix);
  Some(Statement::ForEach { binding,
                            iterable,
                            iterable_type,
                            body,
                            span: span(statement)? })
}

fn typed_pattern<'a>(tree: &'a SyntaxTree, pattern: &'a SyntaxNode) -> Option<(&'a SyntaxNode, Option<Type>)>
{
  if pattern.kind != PATTERN_TYPED
  {
    return Some((pattern, None));
  }
  let inner = pattern.children.first().and_then(|index| tree.nodes.get(*index))?;
  let ty = pattern.children.get(1).and_then(|index| tree.nodes.get(*index))?;
  Some((inner, Some(checked_type(&lower_type(tree, ty))?)))
}

fn lower_binding(tree: &SyntaxTree,
                 pattern: &SyntaxNode,
                 binding_type: &Type,
                 locals: &HashMap<String, Type>,
                 pattern_span: Span,
                 offset: u64)
                 -> Option<(crate::hir::type_check::Local, HashMap<String, Type>, Vec<Statement>)>
{
  if pattern.kind == PATTERN_IDENTIFIER
  {
    let name = first_child_kind(tree, pattern, IDENTIFIER)?.text.clone();
    let binding = local(name.clone(), binding_type.clone(), pattern_span);
    let mut body_locals = locals.clone();
    body_locals.insert(name, binding_type.clone());
    return Some((binding, body_locals, Vec::new()));
  }
  if pattern.kind != PATTERN_TUPLE
  {
    return None;
  }
  let hidden_name = format!("$for_each_tuple_{offset}");
  let binding = local(hidden_name.clone(), binding_type.clone(), pattern_span);
  let mut body_locals = locals.clone();
  body_locals.insert(hidden_name.clone(), binding_type.clone());
  let root = Expression::Local { name: hidden_name,
                                 span: pattern_span };
  let mut prefix = Vec::new();
  let mut binding_names = std::collections::HashSet::new();
  lower_tuple_pattern(tree,
                      pattern,
                      binding_type,
                      root,
                      &mut body_locals,
                      &mut binding_names,
                      &mut prefix)?;
  Some((binding, body_locals, prefix))
}

fn lower_tuple_pattern(tree: &SyntaxTree,
                       pattern: &SyntaxNode,
                       pattern_type: &Type,
                       source: Expression,
                       locals: &mut HashMap<String, Type>,
                       binding_names: &mut std::collections::HashSet<String>,
                       prefix: &mut Vec<Statement>)
                       -> Option<()>
{
  let Type::Tuple { fields } = pattern_type
  else
  {
    return None;
  };
  if pattern.children.len() != fields.len()
  {
    return None;
  }
  for (index, (child, field)) in pattern.children.iter().zip(fields).enumerate()
  {
    let child = tree.nodes.get(*child)?;
    let (child, declared_type) = typed_pattern(tree, child)?;
    if declared_type.as_ref().is_some_and(|declared| declared != &field.ty)
    {
      return None;
    }
    let child_span = span(child)?;
    let projection = Expression::TupleElement { tuple: Box::new(source.clone()),
                                                index: u32::try_from(index).ok()?,
                                                element_type: Box::new(field.ty.clone()),
                                                span: child_span };
    match child.kind
    {
      PATTERN_ELSE =>
      {}
      PATTERN_IDENTIFIER =>
      {
        let name = first_child_kind(tree, child, IDENTIFIER)?.text.clone();
        if !binding_names.insert(name.clone())
        {
          return None;
        }
        locals.insert(name.clone(), field.ty.clone());
        prefix.push(Statement::Let { local: local(name, field.ty.clone(), child_span),
                                     initializer: Some(projection) });
      }
      PATTERN_TUPLE => lower_tuple_pattern(tree, child, &field.ty, projection, locals, binding_names, prefix)?,
      _ => return None,
    }
  }
  Some(())
}

fn local(name: String, ty: Type, span: Span) -> crate::hir::type_check::Local
{
  crate::hir::type_check::Local { name,
                                  ty,
                                  mutable: false,
                                  span }
}
