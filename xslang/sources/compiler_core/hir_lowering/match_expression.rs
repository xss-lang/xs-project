/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

pub(super) fn lower_match_expression(tree: &SyntaxTree,
                                     expression: &SyntaxNode,
                                     signatures: &HashMap<String, CallSignature>,
                                     locals: &HashMap<String, Type>,
                                     result_type: Type)
                                     -> Option<Expression>
{
  let selector_node = tree.nodes.get(*expression.children.first()?)?;
  let selector_type = expression_type(tree, selector_node, signatures, locals)?;
  let selector = lower_expression(tree, selector_node, signatures, locals, Some(&selector_type))?;
  let arms =
    expression.children[1..].iter()
                            .map(|index| lower_arm(tree, *index, signatures, locals, &selector_type, &result_type))
                            .collect::<Option<Vec<_>>>()?;
  Some(Expression::Match { selector: Box::new(selector),
                           selector_type: Box::new(selector_type),
                           arms,
                           result_type: Box::new(result_type),
                           span: span(expression)? })
}

fn lower_arm(tree: &SyntaxTree,
             index: usize,
             signatures: &HashMap<String, CallSignature>,
             locals: &HashMap<String, Type>,
             selector_type: &Type,
             result_type: &Type)
             -> Option<MatchArm>
{
  let arm = tree.nodes.get(index)?;
  if arm.kind != MATCH_ARM || arm.children.len() != 2
  {
    return None;
  }
  let pattern_node = tree.nodes.get(arm.children[0])?;
  let pattern = if pattern_node.kind == PATTERN_ELSE
  {
    MatchPattern::Else
  }
  else if pattern_node.kind == PATTERN_LITERAL
  {
    let literal_node = tree.nodes.get(*pattern_node.children.first()?)?;
    let Expression::Literal { literal, .. } =
      lower_expression(tree, literal_node, signatures, locals, Some(selector_type))?
    else
    {
      return None;
    };
    MatchPattern::Literal(literal)
  }
  else
  {
    return None;
  };
  let mut arm_locals = locals.clone();
  let body = lower_hir_block(tree,
                             tree.nodes.get(arm.children[1])?,
                             signatures,
                             &mut arm_locals,
                             None,
                             Some(result_type))?;
  body.tail.as_ref()?;
  Some(MatchArm { pattern,
                  body,
                  span: span(arm)? })
}
