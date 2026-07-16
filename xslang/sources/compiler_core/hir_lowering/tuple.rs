/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

pub(super) fn lower_tuple_type(tree: &SyntaxTree, value: &SyntaxNode) -> declarations::TypeRef
{
  declarations::TypeRef::Tuple { fields: value.children
                                              .iter()
                                              .filter_map(|index| tree.nodes.get(*index))
                                              .map(|field| {
                                                if field.kind != TUPLE_FIELD
                                                {
                                                  return declarations::TupleFieldRef { name: None,
                                                                                       ty: lower_type(tree, field) };
                                                }
                                                let name = field.children
                                                                .first()
                                                                .and_then(|index| tree.nodes.get(*index))
                                                                .map(|name| name.text.clone());
                                                let ty =
                                                  field.children
                                                       .get(1)
                                                       .and_then(|index| tree.nodes.get(*index))
                                                       .map(|ty| lower_type(tree, ty))
                                                       .unwrap_or_else(|| declarations::TypeRef::Named(String::new()));
                                                declarations::TupleFieldRef { name,
                                                                              ty }
                                              })
                                              .collect() }
}

pub(super) fn tuple_expression_type(tree: &SyntaxTree,
                                    value: &SyntaxNode,
                                    context: &LoweringContext,
                                    locals: &HashMap<String, Type>)
                                    -> Option<Type>
{
  (value.kind == EXPR_TUPLE && !value.children.is_empty()).then_some(())?;
  let fields = value.children
                    .iter()
                    .map(|index| {
                      let field = tree.nodes.get(*index)?;
                      let (name, expression) = tuple_field(tree, field)?;
                      Some(crate::hir::type_check::TupleFieldType { name,
                                                                    ty: expression_type(tree, expression, context,
                                                                                        locals)? })
                    })
                    .collect::<Option<Vec<_>>>()?;
  Some(Type::Tuple { fields })
}

pub(super) fn lower_tuple_expression(tree: &SyntaxTree,
                                     value: &SyntaxNode,
                                     context: &LoweringContext,
                                     locals: &HashMap<String, Type>,
                                     expected_type: Option<&Type>,
                                     source_span: Span)
                                     -> Option<Expression>
{
  let tuple_type = expected_type.cloned()
                                .or_else(|| tuple_expression_type(tree, value, context, locals))?;
  let Type::Tuple { fields: expected_fields, } = &tuple_type
  else
  {
    return None;
  };
  if expected_fields.len() != value.children.len()
  {
    return None;
  }
  let fields = value.children
                    .iter()
                    .zip(expected_fields)
                    .map(|(index, expected)| {
                      let field = tree.nodes.get(*index)?;
                      let (name, expression) = tuple_field(tree, field)?;
                      if name != expected.name
                      {
                        return None;
                      }
                      Some(crate::hir::type_check::TupleFieldValue { name,
                                                                     value: lower_expression(tree,
                                                                                             expression,
                                                                                             context,
                                                                                             locals,
                                                                                             Some(&expected.ty))?,
                                                                     span: span(field)? })
                    })
                    .collect::<Option<Vec<_>>>()?;
  Some(Expression::Tuple { fields,
                           tuple_type: Box::new(tuple_type),
                           span: source_span })
}

pub(super) fn tuple_element_type(tree: &SyntaxTree,
                                 value: &SyntaxNode,
                                 context: &LoweringContext,
                                 locals: &HashMap<String, Type>)
                                 -> Option<Type>
{
  let (_, _, _, element_type) = element_parts(tree, value, context, locals)?;
  Some(element_type)
}

pub(super) fn lower_tuple_element(tree: &SyntaxTree,
                                  value: &SyntaxNode,
                                  context: &LoweringContext,
                                  locals: &HashMap<String, Type>,
                                  source_span: Span)
                                  -> Option<Expression>
{
  let (receiver, tuple_type, index, element_type) = element_parts(tree, value, context, locals)?;
  Some(Expression::TupleElement { tuple: Box::new(lower_expression(tree,
                                                                   receiver,
                                                                   context,
                                                                   locals,
                                                                   Some(&tuple_type))?),
                                  index,
                                  element_type: Box::new(element_type),
                                  span: source_span })
}

fn element_parts<'a>(tree: &'a SyntaxTree,
                     value: &'a SyntaxNode,
                     context: &LoweringContext,
                     locals: &HashMap<String, Type>)
                     -> Option<(&'a SyntaxNode, Type, u32, Type)>
{
  (value.kind == EXPR_MEMBER_ACCESS && value.children.len() == 2).then_some(())?;
  let receiver = tree.nodes.get(value.children[0])?;
  let tuple_type = expression_type(tree, receiver, context, locals)?;
  let Type::Tuple { fields } = &tuple_type
  else
  {
    return None;
  };
  let member = path_text(tree, tree.nodes.get(value.children[1])?);
  let index = member.parse::<usize>().ok().or_else(|| {
                                             fields.iter()
                                                   .position(|field| field.name.as_deref() == Some(member.as_str()))
                                           })?;
  let element_type = fields.get(index)?.ty.clone();
  Some((receiver, tuple_type, u32::try_from(index).ok()?, element_type))
}

fn tuple_field<'a>(tree: &'a SyntaxTree, field: &'a SyntaxNode) -> Option<(Option<String>, &'a SyntaxNode)>
{
  if field.kind == TUPLE_FIELD
  {
    let name = field.children
                    .first()
                    .and_then(|index| tree.nodes.get(*index))?
                    .text
                    .clone();
    let expression = field.children.get(1).and_then(|index| tree.nodes.get(*index))?;
    Some((Some(name), expression))
  }
  else
  {
    Some((None, field))
  }
}
