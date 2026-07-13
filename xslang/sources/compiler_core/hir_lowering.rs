/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use crate::hir::{
  MatchArm, MatchPattern,
  async_check::Span,
  declarations,
  type_check::{BinaryOperator, Block, Expression, Literal, PrimitiveType, Statement, Type, UnaryOperator},
};

use super::{SyntaxNode, SyntaxTree};

mod match_expression;
mod unary;

const FILE: u32 = 0;
const DECL_MODULE: u32 = 1;
const DECL_FUNCTION: u32 = 4;
const DECL_VARIABLE: u32 = 9;
const PARAMETER: u32 = 21;
const IDENTIFIER: u32 = 24;
const PATH: u32 = 25;
const TYPE_NAMED: u32 = 27;
const TYPE_UNIT: u32 = 36;
const STMT_BLOCK: u32 = 38;
const STMT_EXPRESSION: u32 = 39;
const STMT_VARIABLE: u32 = 40;
const STMT_RETURN: u32 = 41;
const STMT_IF: u32 = 42;
const STMT_ELSE_IF: u32 = 43;
const STMT_FOR: u32 = 44;
const STMT_WHILE: u32 = 46;
const STMT_MATCH: u32 = 47;
const MATCH_ARM: u32 = 48;
const STMT_BREAK: u32 = 49;
const STMT_CONTINUE: u32 = 50;
const EXPR_IDENTIFIER: u32 = 56;
const EXPR_LITERAL: u32 = 57;
const EXPR_BINARY: u32 = 58;
const EXPR_UNARY: u32 = 59;
const EXPR_ASSIGNMENT: u32 = 60;
const EXPR_CALL: u32 = 61;
const EXPR_IF: u32 = 81;
const EXPR_MATCH: u32 = 82;
const PATTERN_LITERAL: u32 = 85;
const PATTERN_ELSE: u32 = 88;
const TOKEN_INTEGER: u32 = 3;
const TOKEN_EQUAL: u32 = 25;
const TOKEN_BANG: u32 = 26;
const TOKEN_GREATER: u32 = 32;
const TOKEN_GREATER_EQUAL: u32 = 33;
const TOKEN_LESS: u32 = 35;
const TOKEN_LESS_EQUAL: u32 = 36;
const TOKEN_PLUS: u32 = 38;
const TOKEN_PLUS_PLUS: u32 = 39;
const TOKEN_PLUS_ASSIGN: u32 = 40;
const TOKEN_MINUS: u32 = 41;
const TOKEN_MINUS_MINUS: u32 = 42;
const TOKEN_MINUS_ASSIGN: u32 = 43;
const TOKEN_STAR: u32 = 44;
const TOKEN_STAR_ASSIGN: u32 = 45;
const TOKEN_SLASH: u32 = 46;
const TOKEN_PERCENT: u32 = 48;
const TOKEN_AMPERSAND: u32 = 50;
const TOKEN_PIPE: u32 = 53;
const TOKEN_CARET: u32 = 56;
const TOKEN_SHIFT_RIGHT: u32 = 34;
const TOKEN_SHIFT_LEFT: u32 = 37;
const TOKEN_ASSIGN: u32 = 24;
const IMMUTABLE: u32 = 1 << 4;
const CONSTANT: u32 = 1 << 5;
const STATIC_CONSTANT: u32 = 1 << 6;
const RETURN_TYPE: u32 = 1 << 11;
const INFERRED_TYPE: u32 = 1 << 12;
const DISCARDED: u32 = 1 << 21;
const POST_TEST_LOOP: u32 = 1 << 25;
const FOR_INITIALIZER: u32 = 1 << 27;
const FOR_CONDITION: u32 = 1 << 28;
const FOR_UPDATE: u32 = 1 << 29;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum LoweringError
{
  InvalidRoot,
  MissingIdentifier,
  MissingParameterType,
}

fn node(tree: &SyntaxTree, index: usize) -> Result<&SyntaxNode, LoweringError>
{
  tree.nodes.get(index).ok_or(LoweringError::InvalidRoot)
}

fn first_child_kind<'a>(tree: &'a SyntaxTree, parent: &'a SyntaxNode, kind: u32) -> Option<&'a SyntaxNode>
{
  parent.children
        .iter()
        .filter_map(|index| tree.nodes.get(*index))
        .find(|child| child.kind == kind)
}

fn path_text(tree: &SyntaxTree, value: &SyntaxNode) -> String
{
  let path = if value.kind == PATH
  {
    value
  }
  else
  {
    first_child_kind(tree, value, PATH).unwrap_or(value)
  };
  path.children
      .iter()
      .filter_map(|index| tree.nodes.get(*index))
      .filter(|child| child.kind == IDENTIFIER)
      .map(|child| child.text.as_str())
      .collect::<Vec<_>>()
      .join("::")
}

fn primitive(name: &str) -> Option<PrimitiveType>
{
  Some(match name
  {
    "Bool" => PrimitiveType::Bool,
    "Byte" => PrimitiveType::Byte,
    "SByte" => PrimitiveType::SByte,
    "Char" => PrimitiveType::Char,
    "Short" => PrimitiveType::Short,
    "Long" => PrimitiveType::Long,
    "Int" => PrimitiveType::Int,
    "Integer" => PrimitiveType::Integer,
    "UShort" => PrimitiveType::UShort,
    "ULong" => PrimitiveType::ULong,
    "UInt" => PrimitiveType::UInt,
    "UInteger" => PrimitiveType::UInteger,
    "SFloat" => PrimitiveType::SFloat,
    "Float" => PrimitiveType::Float,
    "Str" => PrimitiveType::Str,
    _ => return None,
  })
}

fn lower_type(tree: &SyntaxTree, value: &SyntaxNode) -> declarations::TypeRef
{
  if value.kind == TYPE_UNIT
  {
    return declarations::TypeRef::Unit;
  }
  if value.kind == TYPE_NAMED
  {
    let name = path_text(tree, value);
    if let Some(primitive) = primitive(&name)
    {
      return declarations::TypeRef::Primitive(primitive);
    }
    return declarations::TypeRef::Named(name);
  }
  declarations::TypeRef::Named(value.text.clone())
}

fn checked_type(value: &declarations::TypeRef) -> Option<crate::hir::type_check::Type>
{
  Some(match value
  {
    declarations::TypeRef::Unit => return None,
    declarations::TypeRef::Primitive(value) => crate::hir::type_check::Type::Primitive(*value),
    declarations::TypeRef::Named(value) => crate::hir::type_check::Type::Named(value.clone()),
  })
}

fn lower_parameter(tree: &SyntaxTree, value: &SyntaxNode) -> Result<declarations::Parameter, LoweringError>
{
  let name = first_child_kind(tree, value, IDENTIFIER).ok_or(LoweringError::MissingIdentifier)?;
  let ty = value.children
                .iter()
                .filter_map(|index| tree.nodes.get(*index))
                .find(|child| child.kind != IDENTIFIER)
                .ok_or(LoweringError::MissingParameterType)?;
  Ok(declarations::Parameter { name: name.text.clone(),
                               ty: lower_type(tree, ty),
                               span: value.span.clone() })
}

fn span(value: &SyntaxNode) -> Option<Span>
{
  Some(Span::new(value.span.file_id,
                 u32::try_from(value.span.start_offset).ok()?,
                 u32::try_from(value.span.end_offset).ok()?))
}

#[derive(Clone)]
struct CallSignature
{
  parameters: Vec<crate::hir::type_check::Type>,
  return_type: crate::hir::type_check::Type,
}

fn lower_expression(tree: &SyntaxTree,
                    value: &SyntaxNode,
                    signatures: &HashMap<String, CallSignature>,
                    locals: &HashMap<String, Type>,
                    expected_type: Option<&Type>)
                    -> Option<Expression>
{
  let source_span = span(value)?;
  match value.kind
  {
    EXPR_LITERAL if value.token_kind == TOKEN_INTEGER =>
    {
      Some(Expression::Literal { literal: Literal::Integer(value.text.clone()),
                                 span: source_span })
    }
    EXPR_LITERAL if value.text == "true" || value.text == "false" =>
    {
      Some(Expression::Literal { literal: Literal::Bool(value.text == "true"),
                                 span: source_span })
    }
    EXPR_IDENTIFIER => Some(Expression::Local { name: path_text(tree, value),
                                                span: source_span }),
    EXPR_UNARY if value.children.len() == 1 =>
    {
      unary::lower_unary_expression(tree, value, signatures, locals, expected_type, source_span)
    }
    EXPR_BINARY if value.children.len() == 3 =>
    {
      let operator = match value.token_kind
      {
        TOKEN_PLUS => BinaryOperator::Add,
        TOKEN_MINUS => BinaryOperator::Sub,
        TOKEN_STAR => BinaryOperator::Mul,
        TOKEN_SLASH => BinaryOperator::Div,
        TOKEN_PERCENT => BinaryOperator::Rem,
        TOKEN_AMPERSAND => BinaryOperator::BitAnd,
        TOKEN_PIPE => BinaryOperator::BitOr,
        TOKEN_CARET => BinaryOperator::BitXor,
        TOKEN_SHIFT_LEFT => BinaryOperator::ShiftLeft,
        TOKEN_SHIFT_RIGHT => BinaryOperator::ShiftRight,
        TOKEN_EQUAL => BinaryOperator::Equal,
        TOKEN_LESS => BinaryOperator::Less,
        TOKEN_LESS_EQUAL => BinaryOperator::LessEqual,
        TOKEN_GREATER => BinaryOperator::Greater,
        TOKEN_GREATER_EQUAL => BinaryOperator::GreaterEqual,
        _ => return None,
      };
      let operand_type = if matches!(operator,
                                     BinaryOperator::Add |
                                     BinaryOperator::Sub |
                                     BinaryOperator::Mul |
                                     BinaryOperator::Div |
                                     BinaryOperator::Rem |
                                     BinaryOperator::BitAnd |
                                     BinaryOperator::BitOr |
                                     BinaryOperator::BitXor |
                                     BinaryOperator::ShiftLeft |
                                     BinaryOperator::ShiftRight)
      {
        expected_type
      }
      else
      {
        None
      };
      let left = lower_expression(tree,
                                  tree.nodes.get(value.children[0])?,
                                  signatures,
                                  locals,
                                  operand_type)?;
      let right = lower_expression(tree,
                                   tree.nodes.get(value.children[2])?,
                                   signatures,
                                   locals,
                                   operand_type)?;
      Some(Expression::Binary { operator,
                                left: Box::new(left),
                                right: Box::new(right),
                                span: source_span })
    }
    EXPR_ASSIGNMENT if value.children.len() == 3 =>
    {
      let target = tree.nodes.get(value.children[0])?;
      if target.kind != EXPR_IDENTIFIER
      {
        return None;
      }
      let target = path_text(tree, target);
      let assigned = lower_expression(tree,
                                      tree.nodes.get(value.children[2])?,
                                      signatures,
                                      locals,
                                      locals.get(&target))?;
      let assigned = match value.token_kind
      {
        TOKEN_ASSIGN => assigned,
        TOKEN_PLUS_ASSIGN | TOKEN_MINUS_ASSIGN | TOKEN_STAR_ASSIGN =>
        {
          let operator = match value.token_kind
          {
            TOKEN_PLUS_ASSIGN => BinaryOperator::Add,
            TOKEN_MINUS_ASSIGN => BinaryOperator::Sub,
            TOKEN_STAR_ASSIGN => BinaryOperator::Mul,
            _ => return None,
          };
          Expression::Binary { operator,
                               left: Box::new(Expression::Local { name: target.clone(),
                                                                  span: source_span }),
                               right: Box::new(assigned),
                               span: source_span }
        }
        _ => return None,
      };
      Some(Expression::Assign { target,
                                value: Box::new(assigned),
                                span: source_span })
    }
    EXPR_CALL if !value.children.is_empty() =>
    {
      let callee = tree.nodes.get(value.children[0])?;
      if callee.kind != EXPR_IDENTIFIER
      {
        return None;
      }
      let function = path_text(tree, callee);
      let signature = signatures.get(&function)?;
      if value.children.len() - 1 != signature.parameters.len()
      {
        return None;
      }
      let arguments =
        value.children[1..].iter()
                           .zip(&signature.parameters)
                           .map(|(index, parameter)| {
                             lower_expression(tree, tree.nodes.get(*index)?, signatures, locals, Some(parameter))
                           })
                           .collect::<Option<Vec<_>>>()?;
      Some(Expression::Call { function,
                              arguments,
                              parameter_types: signature.parameters.clone(),
                              return_type: Box::new(signature.return_type.clone()),
                              span: source_span })
    }
    EXPR_IF if value.children.len() == 3 =>
    {
      let result_type = expected_type?.clone();
      let condition = lower_expression(tree,
                                       tree.nodes.get(value.children[0])?,
                                       signatures,
                                       locals,
                                       Some(&Type::Primitive(PrimitiveType::Bool)))?;
      let mut then_locals = locals.clone();
      let then_block = lower_hir_block(tree,
                                       tree.nodes.get(value.children[1])?,
                                       signatures,
                                       &mut then_locals,
                                       None,
                                       Some(&result_type))?;
      let mut else_locals = locals.clone();
      let else_block = lower_hir_block(tree,
                                       tree.nodes.get(value.children[2])?,
                                       signatures,
                                       &mut else_locals,
                                       None,
                                       Some(&result_type))?;
      if then_block.tail.is_none() || else_block.tail.is_none()
      {
        return None;
      }
      Some(Expression::If { condition: Box::new(condition),
                            then_block: Box::new(then_block),
                            else_block: Box::new(else_block),
                            result_type: Box::new(result_type),
                            span: source_span })
    }
    EXPR_MATCH if value.children.len() >= 2 =>
    {
      match_expression::lower_match_expression(tree, value, signatures, locals, expected_type?.clone())
    }
    _ => None,
  }
}

fn lower_discarded_expression(tree: &SyntaxTree,
                              value: &SyntaxNode,
                              signatures: &HashMap<String, CallSignature>,
                              locals: &HashMap<String, Type>,
                              expected_type: Option<&Type>)
                              -> Option<Expression>
{
  if value.kind == EXPR_UNARY &&
     matches!(value.token_kind, TOKEN_PLUS_PLUS | TOKEN_MINUS_MINUS) &&
     value.children.len() == 1
  {
    let target = tree.nodes.get(value.children[0])?;
    if target.kind != EXPR_IDENTIFIER ||
       locals.get(&path_text(tree, target)) != Some(&Type::Primitive(PrimitiveType::Long))
    {
      return None;
    }
    let target = path_text(tree, target);
    let source_span = span(value)?;
    let operator = if value.token_kind == TOKEN_PLUS_PLUS
    {
      BinaryOperator::Add
    }
    else
    {
      BinaryOperator::Sub
    };
    return Some(Expression::Assign {
      target: target.clone(),
      value: Box::new(Expression::Binary {
        operator,
        left: Box::new(Expression::Local { name: target,
                                           span: source_span }),
        right: Box::new(Expression::Literal { literal: Literal::Integer("1".to_string()),
                                              span: source_span }),
        span: source_span,
      }),
      span: source_span,
    });
  }
  lower_expression(tree, value, signatures, locals, expected_type)
}

fn expression_type(tree: &SyntaxTree,
                   value: &SyntaxNode,
                   signatures: &HashMap<String, CallSignature>,
                   locals: &HashMap<String, Type>)
                   -> Option<Type>
{
  match value.kind
  {
    EXPR_LITERAL if value.text == "true" || value.text == "false" => Some(Type::Primitive(PrimitiveType::Bool)),
    EXPR_LITERAL if value.token_kind == TOKEN_INTEGER => Some(Type::Primitive(PrimitiveType::Long)),
    EXPR_IDENTIFIER => locals.get(&path_text(tree, value)).cloned(),
    EXPR_CALL =>
    {
      let callee = tree.nodes.get(*value.children.first()?)?;
      signatures.get(&path_text(tree, callee))
                .map(|signature| signature.return_type.clone())
    }
    EXPR_BINARY
      if matches!(value.token_kind,
                  TOKEN_EQUAL | TOKEN_GREATER | TOKEN_GREATER_EQUAL | TOKEN_LESS | TOKEN_LESS_EQUAL) =>
    {
      Some(Type::Primitive(PrimitiveType::Bool))
    }
    EXPR_BINARY => Some(Type::Primitive(PrimitiveType::Long)),
    EXPR_UNARY if value.token_kind == TOKEN_BANG => Some(Type::Primitive(PrimitiveType::Bool)),
    EXPR_UNARY => expression_type(tree, tree.nodes.get(*value.children.first()?)?, signatures, locals),
    _ => None,
  }
}

fn lower_local(tree: &SyntaxTree,
               statement: &SyntaxNode,
               signatures: &HashMap<String, CallSignature>,
               locals: &mut HashMap<String, Type>)
               -> Option<Statement>
{
  let declaration = if statement.kind == DECL_VARIABLE
  {
    statement
  }
  else
  {
    first_child_kind(tree, statement, DECL_VARIABLE)?
  };
  if declaration.flags & INFERRED_TYPE != 0
  {
    return None;
  }
  let name = first_child_kind(tree, declaration, IDENTIFIER)?;
  let ty = declaration.children
                      .iter()
                      .filter_map(|index| tree.nodes.get(*index))
                      .find(|child| (TYPE_NAMED..=TYPE_UNIT).contains(&child.kind))?;
  let initializer_node = declaration.children
                                    .iter()
                                    .filter_map(|index| tree.nodes.get(*index))
                                    .find(|child| child.kind >= EXPR_IDENTIFIER);
  let checked_type = checked_type(&lower_type(tree, ty))?;
  let initializer = match initializer_node
  {
    Some(value) => Some(lower_expression(tree, value, signatures, locals, Some(&checked_type))?),
    None => None,
  };
  locals.insert(name.text.clone(), checked_type.clone());
  Some(Statement::Let { local: crate::hir::type_check::Local { name: name.text.clone(),
                                                               ty: checked_type,
                                                               mutable: declaration.flags &
                                                                        (IMMUTABLE |
                                                                         CONSTANT |
                                                                         STATIC_CONSTANT) ==
                                                                        0,
                                                               span: span(declaration)? },
                        initializer })
}

fn lower_statement_node(tree: &SyntaxTree,
                        statement: &SyntaxNode,
                        signatures: &HashMap<String, CallSignature>,
                        locals: &mut HashMap<String, Type>,
                        return_type: Option<&Type>)
                        -> Option<Statement>
{
  match statement.kind
  {
    STMT_RETURN =>
    {
      let value = match statement.children.first().and_then(|index| tree.nodes.get(*index))
      {
        Some(value) => Some(lower_expression(tree, value, signatures, locals, return_type)?),
        None => None,
      };
      Some(Statement::Return { value,
                               span: span(statement)? })
    }
    STMT_EXPRESSION if statement.children.len() == 1 =>
    {
      let expression = tree.nodes.get(statement.children[0])?;
      let expected = if expression.kind == EXPR_ASSIGNMENT
      {
        expression.children
                  .first()
                  .and_then(|index| tree.nodes.get(*index))
                  .map(|target| path_text(tree, target))
                  .and_then(|target| locals.get(&target))
      }
      else
      {
        None
      };
      lower_discarded_expression(tree, expression, signatures, locals, expected).map(Statement::Expr)
    }
    STMT_VARIABLE => lower_local(tree, statement, signatures, locals),
    STMT_IF => lower_if_statement(tree, statement, signatures, locals, return_type),
    STMT_FOR => lower_for_statement(tree, statement, signatures, locals, return_type),
    STMT_WHILE => lower_while_statement(tree, statement, signatures, locals, return_type),
    STMT_MATCH => lower_match_statement(tree, statement, signatures, locals, return_type),
    STMT_BREAK => Some(Statement::Break { span: span(statement)? }),
    STMT_CONTINUE => Some(Statement::Continue { span: span(statement)? }),
    _ => None,
  }
}

fn lower_hir_block(tree: &SyntaxTree,
                   block: &SyntaxNode,
                   signatures: &HashMap<String, CallSignature>,
                   locals: &mut HashMap<String, Type>,
                   return_type: Option<&Type>,
                   tail_type: Option<&Type>)
                   -> Option<Block>
{
  if block.kind != STMT_BLOCK
  {
    return None;
  }
  let mut statements = Vec::with_capacity(block.children.len());
  let mut tail = None;
  for (position, child_index) in block.children.iter().enumerate()
  {
    let statement = tree.nodes.get(*child_index)?;
    if position + 1 == block.children.len() && statement.kind == STMT_EXPRESSION && statement.flags & DISCARDED == 0
    {
      let expression = tree.nodes.get(*statement.children.first()?)?;
      tail = Some(Box::new(lower_expression(tree, expression, signatures, locals, tail_type)?));
    }
    else
    {
      statements.push(lower_statement_node(tree, statement, signatures, locals, return_type)?);
    }
  }
  Some(Block { statements,
               tail,
               span: span(block)? })
}

fn lower_if_branch(tree: &SyntaxTree,
                   branch: &SyntaxNode,
                   signatures: &HashMap<String, CallSignature>,
                   locals: &HashMap<String, Type>,
                   return_type: Option<&Type>,
                   else_block: Option<Block>)
                   -> Option<Block>
{
  let condition = lower_expression(tree,
                                   tree.nodes.get(*branch.children.first()?)?,
                                   signatures,
                                   locals,
                                   Some(&Type::Primitive(PrimitiveType::Bool)))?;
  let mut branch_locals = locals.clone();
  let then_block = lower_hir_block(tree,
                                   tree.nodes.get(*branch.children.get(1)?)?,
                                   signatures,
                                   &mut branch_locals,
                                   return_type,
                                   None)?;
  let nested = Statement::If { condition,
                               then_block,
                               else_block,
                               span: span(branch)? };
  Some(Block { statements: vec![nested],
               tail: None,
               span: span(branch)? })
}

fn lower_if_statement(tree: &SyntaxTree,
                      statement: &SyntaxNode,
                      signatures: &HashMap<String, CallSignature>,
                      locals: &HashMap<String, Type>,
                      return_type: Option<&Type>)
                      -> Option<Statement>
{
  let condition = lower_expression(tree,
                                   tree.nodes.get(*statement.children.first()?)?,
                                   signatures,
                                   locals,
                                   Some(&Type::Primitive(PrimitiveType::Bool)))?;
  let mut then_locals = locals.clone();
  let then_block = lower_hir_block(tree,
                                   tree.nodes.get(*statement.children.get(1)?)?,
                                   signatures,
                                   &mut then_locals,
                                   return_type,
                                   None)?;
  let mut else_block = match statement.children[2..].last()
                                                    .and_then(|index| tree.nodes.get(*index))
                                                    .filter(|node| node.kind == STMT_BLOCK)
  {
    Some(block) =>
    {
      let mut branch_locals = locals.clone();
      Some(lower_hir_block(tree, block, signatures, &mut branch_locals, return_type, None)?)
    }
    None => None,
  };
  for branch in statement.children[2..].iter()
                                       .filter_map(|index| tree.nodes.get(*index))
                                       .filter(|node| node.kind == STMT_ELSE_IF)
                                       .rev()
  {
    else_block = Some(lower_if_branch(tree, branch, signatures, locals, return_type, else_block)?);
  }
  Some(Statement::If { condition,
                       then_block,
                       else_block,
                       span: span(statement)? })
}

fn lower_while_statement(tree: &SyntaxTree,
                         statement: &SyntaxNode,
                         signatures: &HashMap<String, CallSignature>,
                         locals: &HashMap<String, Type>,
                         return_type: Option<&Type>)
                         -> Option<Statement>
{
  let condition = lower_expression(tree,
                                   tree.nodes.get(*statement.children.first()?)?,
                                   signatures,
                                   locals,
                                   Some(&Type::Primitive(PrimitiveType::Bool)))?;
  let mut body_locals = locals.clone();
  let mut body = lower_hir_block(tree,
                                 tree.nodes.get(*statement.children.get(1)?)?,
                                 signatures,
                                 &mut body_locals,
                                 return_type,
                                 None)?;
  let condition = if statement.flags & POST_TEST_LOOP != 0
  {
    let loop_span = span(statement)?;
    body.statements.push(Statement::If { condition,
                                         then_block: Block { statements: Vec::new(),
                                                             tail: None,
                                                             span: loop_span },
                                         else_block: Some(Block { statements:
                                                                    vec![Statement::Break { span: loop_span }],
                                                                  tail: None,
                                                                  span: loop_span }),
                                         span: loop_span });
    Expression::Literal { literal: Literal::Bool(true),
                          span: span(statement)? }
  }
  else
  {
    condition
  };
  Some(Statement::While { condition,
                          body,
                          span: span(statement)? })
}

fn lower_for_statement(tree: &SyntaxTree,
                       statement: &SyntaxNode,
                       signatures: &HashMap<String, CallSignature>,
                       locals: &HashMap<String, Type>,
                       return_type: Option<&Type>)
                       -> Option<Statement>
{
  let body_node = statement.children.last().and_then(|index| tree.nodes.get(*index))?;
  if body_node.kind != STMT_BLOCK
  {
    return None;
  }
  let mut for_locals = locals.clone();
  let mut cursor = 0usize;
  let initializer = if statement.flags & FOR_INITIALIZER != 0
  {
    let node = tree.nodes.get(*statement.children.get(cursor)?)?;
    cursor += 1;
    let lowered = if node.kind == DECL_VARIABLE
    {
      lower_local(tree, node, signatures, &mut for_locals)?
    }
    else
    {
      Statement::Expr(lower_discarded_expression(tree, node, signatures, &for_locals, None)?)
    };
    Some(Box::new(lowered))
  }
  else
  {
    None
  };
  let condition = if statement.flags & FOR_CONDITION != 0
  {
    let node = tree.nodes.get(*statement.children.get(cursor)?)?;
    cursor += 1;
    Some(lower_expression(tree,
                          node,
                          signatures,
                          &for_locals,
                          Some(&Type::Primitive(PrimitiveType::Bool)))?)
  }
  else
  {
    None
  };
  let update = if statement.flags & FOR_UPDATE != 0
  {
    let node = tree.nodes.get(*statement.children.get(cursor)?)?;
    cursor += 1;
    Some(lower_discarded_expression(tree, node, signatures, &for_locals, None)?)
  }
  else
  {
    None
  };
  if statement.children.get(cursor).copied() != statement.children.last().copied()
  {
    return None;
  }
  let mut body_locals = for_locals;
  let body = lower_hir_block(tree, body_node, signatures, &mut body_locals, return_type, None)?;
  Some(Statement::For { initializer,
                        condition,
                        update,
                        body,
                        span: span(statement)? })
}

fn lower_match_statement(tree: &SyntaxTree,
                         statement: &SyntaxNode,
                         signatures: &HashMap<String, CallSignature>,
                         locals: &HashMap<String, Type>,
                         return_type: Option<&Type>)
                         -> Option<Statement>
{
  let selector_node = tree.nodes.get(*statement.children.first()?)?;
  let selector_type = expression_type(tree, selector_node, signatures, locals)?;
  let selector = lower_expression(tree, selector_node, signatures, locals, Some(&selector_type))?;
  let arms =
    statement.children[1..].iter()
                           .map(|index| {
                             let arm = tree.nodes.get(*index)?;
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
                                 lower_expression(tree, literal_node, signatures, locals, Some(&selector_type))?
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
                                                        return_type,
                                                        None)?;
                             Some(MatchArm { pattern,
                                             body,
                                             span: span(arm)? })
                           })
                           .collect::<Option<Vec<_>>>()?;
  Some(Statement::Match { selector,
                          selector_type,
                          arms,
                          span: span(statement)? })
}

fn lower_body(tree: &SyntaxTree,
              function: &SyntaxNode,
              signatures: &HashMap<String, CallSignature>,
              parameters: &[declarations::Parameter],
              return_type: Option<&Type>)
              -> Option<Vec<Statement>>
{
  let mut locals = parameters.iter()
                             .filter_map(|parameter| checked_type(&parameter.ty).map(|ty| (parameter.name.clone(), ty)))
                             .collect::<HashMap<_, _>>();
  let block = first_child_kind(tree, function, STMT_BLOCK)?;
  let block = lower_hir_block(tree, block, signatures, &mut locals, return_type, return_type)?;
  let mut body = block.statements;
  if let Some(tail) = block.tail
  {
    body.push(Statement::Return { value: Some(*tail),
                                  span: block.span });
  }
  Some(body)
}

fn lower_function_signature(tree: &SyntaxTree, value: &SyntaxNode) -> Result<declarations::Function, LoweringError>
{
  let name = first_child_kind(tree, value, IDENTIFIER).ok_or(LoweringError::MissingIdentifier)?;
  let parameters = value.children
                        .iter()
                        .filter_map(|index| tree.nodes.get(*index))
                        .filter(|child| child.kind == PARAMETER)
                        .map(|parameter| lower_parameter(tree, parameter))
                        .collect::<Result<_, _>>()?;
  let return_type = value.children
                         .iter()
                         .filter_map(|index| tree.nodes.get(*index))
                         .find(|child| child.flags & RETURN_TYPE != 0)
                         .map_or(declarations::TypeRef::Unit, |ty| lower_type(tree, ty));
  Ok(declarations::Function { name: name.text.clone(),
                              parameters,
                              return_type,
                              flags: value.flags,
                              span: value.span.clone(),
                              body_present: first_child_kind(tree, value, STMT_BLOCK).is_some(),
                              body: None })
}

pub fn lower_declarations(tree: &SyntaxTree) -> Result<declarations::Module, LoweringError>
{
  let root = node(tree, tree.root)?;
  if root.kind != FILE
  {
    return Err(LoweringError::InvalidRoot);
  }
  let name = root.children
                 .iter()
                 .filter_map(|index| tree.nodes.get(*index))
                 .find(|child| child.kind == DECL_MODULE)
                 .and_then(|module| first_child_kind(tree, module, PATH))
                 .map(|path| path_text(tree, path));
  let function_nodes = root.children
                           .iter()
                           .filter_map(|index| tree.nodes.get(*index))
                           .filter(|child| child.kind == DECL_FUNCTION)
                           .collect::<Vec<_>>();
  let mut functions = function_nodes.iter()
                                    .map(|function| lower_function_signature(tree, function))
                                    .collect::<Result<Vec<_>, _>>()?;
  let signatures = functions.iter()
                            .filter_map(|function| {
                              let parameters = function.parameters
                                                       .iter()
                                                       .map(|parameter| checked_type(&parameter.ty))
                                                       .collect::<Option<Vec<_>>>()?;
                              let return_type = checked_type(&function.return_type)?;
                              Some((function.name.clone(),
                                    CallSignature { parameters,
                                                    return_type }))
                            })
                            .collect::<HashMap<_, _>>();
  for (function, syntax) in functions.iter_mut().zip(function_nodes)
  {
    let parameters = function.parameters.clone();
    let return_type = checked_type(&function.return_type);
    function.body = lower_body(tree, syntax, &signatures, &parameters, return_type.as_ref());
  }
  Ok(declarations::Module { name,
                            functions })
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::compiler_core::SourceSpan;

  fn syntax(kind: u32, text: &str, parent: Option<usize>, children: Vec<usize>) -> SyntaxNode
  {
    SyntaxNode { kind,
                 token_kind: 0,
                 visibility: 0,
                 flags: 0,
                 parent,
                 children,
                 text: text.to_owned(),
                 span: SourceSpan { file_id: 1,
                                    start_offset: 0,
                                    end_offset: text.len() as u64,
                                    start_line: 1,
                                    start_column: 1,
                                    end_line: 1,
                                    end_column: text.len() as u64 + 1 } }
  }

  #[test]
  fn lowers_module_and_function_signature()
  {
    let mut nodes = vec![syntax(FILE, "", None, vec![1, 4]),
                         syntax(DECL_MODULE, "module app;", Some(0), vec![2]),
                         syntax(PATH, "app", Some(1), vec![3]),
                         syntax(IDENTIFIER, "app", Some(2), vec![]),
                         syntax(DECL_FUNCTION, "fn main(value: Long) -> Long", Some(0), vec![5, 6, 9]),
                         syntax(IDENTIFIER, "main", Some(4), vec![]),
                         syntax(PARAMETER, "value: Long", Some(4), vec![7, 8]),
                         syntax(IDENTIFIER, "value", Some(6), vec![]),
                         syntax(TYPE_NAMED, "Long", Some(6), vec![10]),
                         syntax(TYPE_NAMED, "Long", Some(4), vec![11])];
    nodes.push(syntax(PATH, "Long", Some(8), vec![12]));
    nodes.push(syntax(PATH, "Long", Some(9), vec![13]));
    nodes.push(syntax(IDENTIFIER, "Long", Some(10), vec![]));
    nodes.push(syntax(IDENTIFIER, "Long", Some(11), vec![]));
    nodes[9].flags = RETURN_TYPE;
    let module = lower_declarations(&SyntaxTree { root: 0,
                                                  nodes }).expect("signature module");
    assert_eq!(module.name.as_deref(), Some("app"));
    assert_eq!(module.functions[0].name, "main");
    assert_eq!(module.functions[0].parameters[0].ty,
               declarations::TypeRef::Primitive(PrimitiveType::Long));
    assert_eq!(module.functions[0].return_type,
               declarations::TypeRef::Primitive(PrimitiveType::Long));
  }

  #[test]
  fn lowers_long_return_body_for_hir_to_mir()
  {
    let mut nodes = vec![syntax(FILE, "", None, vec![1]),
                         syntax(DECL_FUNCTION, "fn main() -> Long { return 7; }", Some(0), vec![2, 3, 6]),
                         syntax(IDENTIFIER, "main", Some(1), vec![]),
                         syntax(TYPE_NAMED, "Long", Some(1), vec![4]),
                         syntax(PATH, "Long", Some(3), vec![5]),
                         syntax(IDENTIFIER, "Long", Some(4), vec![]),
                         syntax(STMT_BLOCK, "{ return 7; }", Some(1), vec![7]),
                         syntax(STMT_RETURN, "return 7;", Some(6), vec![8]),
                         syntax(EXPR_LITERAL, "7", Some(7), vec![])];
    nodes[3].flags = RETURN_TYPE;
    nodes[8].token_kind = TOKEN_INTEGER;
    let module = lower_declarations(&SyntaxTree { root: 0,
                                                  nodes }).expect("body module");
    let hir = module.functions[0].as_type_checked_input().expect("HIR body");
    assert!(crate::hir::type_check::TypeChecker::new().check_function(&hir)
                                                      .is_empty());
    let mir = crate::hir::mir_lowering::HirToMirLowerer::new().lower_function(&hir)
                                                              .expect("MIR body");
    assert!(matches!(mir.blocks[0].statements[0],
                     crate::mir::Statement::ConstI32 { value: 7,
                                                       .. }));
  }
}
