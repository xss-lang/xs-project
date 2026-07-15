/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;
use crate::hir::type_check::MapEntry;

const EXPR_INDEX: u32 = 68;
const EXPR_ARRAY_LITERAL: u32 = 76;
const EXPR_MAP_LITERAL: u32 = 107;

pub(super) const fn is_expression(kind: u32) -> bool
{
  matches!(kind, EXPR_INDEX | EXPR_ARRAY_LITERAL | EXPR_MAP_LITERAL)
}

pub(super) fn expression_type(tree: &SyntaxTree,
                              value: &SyntaxNode,
                              context: &LoweringContext,
                              locals: &HashMap<String, Type>)
                              -> Option<Type>
{
  match value.kind
  {
    EXPR_ARRAY_LITERAL =>
    {
      let first = tree.nodes.get(*value.children.first()?)?;
      let element = super::expression_type(tree, first, context, locals)?;
      value.children
           .iter()
           .skip(1)
           .all(|index| {
             tree.nodes
                 .get(*index)
                 .and_then(|node| super::expression_type(tree, node, context, locals))
                 .as_ref() ==
             Some(&element)
           })
           .then(|| Type::Array { element: Box::new(element),
                                  length: u64::try_from(value.children.len()).ok() })
    }
    EXPR_MAP_LITERAL => map_type(tree, value, context, locals),
    EXPR_INDEX =>
    {
      let collection = tree.nodes.get(*value.children.first()?)?;
      match super::expression_type(tree, collection, context, locals)?
      {
        Type::Array { element, .. } => Some(*element),
        _ => None,
      }
    }
    _ => None,
  }
}

pub(super) fn lower_expression(tree: &SyntaxTree,
                               value: &SyntaxNode,
                               context: &LoweringContext,
                               locals: &HashMap<String, Type>,
                               expected_type: Option<&Type>,
                               span: Span)
                               -> Option<Expression>
{
  match value.kind
  {
    EXPR_ARRAY_LITERAL => lower_array(tree, value, context, locals, expected_type, span),
    EXPR_MAP_LITERAL => lower_map(tree, value, context, locals, expected_type, span),
    EXPR_INDEX if value.children.len() == 2 =>
    {
      let collection_node = tree.nodes.get(value.children[0])?;
      let collection_type = super::expression_type(tree, collection_node, context, locals)?;
      let Type::Array { element, .. } = &collection_type
      else
      {
        return None;
      };
      let collection = super::lower_expression(tree, collection_node, context, locals, Some(&collection_type))?;
      let index = super::lower_expression(tree,
                                          tree.nodes.get(value.children[1])?,
                                          context,
                                          locals,
                                          Some(&Type::Primitive(PrimitiveType::Int)))?;
      Some(Expression::Index { collection: Box::new(collection),
                               index: Box::new(index),
                               element_type: element.clone(),
                               span })
    }
    _ => None,
  }
}

fn lower_array(tree: &SyntaxTree,
               value: &SyntaxNode,
               context: &LoweringContext,
               locals: &HashMap<String, Type>,
               expected_type: Option<&Type>,
               span: Span)
               -> Option<Expression>
{
  let inferred = expression_type(tree, value, context, locals);
  let array_type = expected_type.or(inferred.as_ref())?;
  let Type::Array { element,
                    length, } = array_type
  else
  {
    return None;
  };
  if length.is_some_and(|length| u64::try_from(value.children.len()).ok() != Some(length))
  {
    return None;
  }
  let elements =
    value.children
         .iter()
         .map(|index| super::lower_expression(tree, tree.nodes.get(*index)?, context, locals, Some(element)))
         .collect::<Option<Vec<_>>>()?;
  Some(Expression::Array { elements,
                           span })
}

fn lower_map(tree: &SyntaxTree,
             node: &SyntaxNode,
             context: &LoweringContext,
             locals: &HashMap<String, Type>,
             expected_type: Option<&Type>,
             span: Span)
             -> Option<Expression>
{
  let inferred = map_type(tree, node, context, locals);
  let Type::Map { key,
                  value, } = expected_type.or(inferred.as_ref())?
  else
  {
    return None;
  };
  let entries = node.children
                    .iter()
                    .map(|index| {
                      let entry = tree.nodes.get(*index)?;
                      Some(MapEntry { key: super::lower_expression(tree,
                                                                   tree.nodes.get(*entry.children.first()?)?,
                                                                   context,
                                                                   locals,
                                                                   Some(key))?,
                                      value: super::lower_expression(tree,
                                                                     tree.nodes.get(*entry.children.get(1)?)?,
                                                                     context,
                                                                     locals,
                                                                     Some(value))?,
                                      span: super::span(entry)? })
                    })
                    .collect::<Option<Vec<_>>>()?;
  Some(Expression::Map { entries,
                         span })
}

fn map_type(tree: &SyntaxTree,
            node: &SyntaxNode,
            context: &LoweringContext,
            locals: &HashMap<String, Type>)
            -> Option<Type>
{
  let first = tree.nodes.get(*node.children.first()?)?;
  let key = super::expression_type(tree, tree.nodes.get(*first.children.first()?)?, context, locals)?;
  let value = super::expression_type(tree, tree.nodes.get(*first.children.get(1)?)?, context, locals)?;
  node.children
      .iter()
      .skip(1)
      .all(|index| {
        let Some(entry) = tree.nodes.get(*index)
        else
        {
          return false;
        };
        super::expression_type(tree, &tree.nodes[entry.children[0]], context, locals).as_ref() == Some(&key) &&
        super::expression_type(tree, &tree.nodes[entry.children[1]], context, locals).as_ref() == Some(&value)
      })
      .then(|| Type::Map { key: Box::new(key),
                           value: Box::new(value) })
}
