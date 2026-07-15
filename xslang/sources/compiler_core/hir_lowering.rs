/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::HashMap;

use crate::hir::{
  MatchArm, MatchPattern,
  async_check::Span,
  declarations,
  type_check::{
    BinaryOperator, Block, Expression, Literal, PrimitiveType, Statement, Type, UnaryOperator, UpdateOperator,
    UpdatePosition,
  },
};

use super::{SyntaxNode, SyntaxTree};

mod expression_type;
mod match_expression;
mod nominal;
mod program;
mod unary;

use expression_type::expression_type;

const FILE: u32 = 0;
const DECL_MODULE: u32 = 1;
const DECL_FUNCTION: u32 = 4;
const DECL_CLASS: u32 = 5;
const DECL_DATA: u32 = 8;
const DECL_VARIABLE: u32 = 9;
const CLASS_FIELD: u32 = 15;
const DATA_FIELD: u32 = 20;
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
const EXPR_MEMBER_ACCESS: u32 = 63;
const EXPR_OBJECT_LITERAL: u32 = 77;
const OBJECT_FIELD: u32 = 78;
const EXPR_IF: u32 = 81;
const EXPR_MATCH: u32 = 82;
const PATTERN_LITERAL: u32 = 85;
const PATTERN_ELSE: u32 = 88;
const STMT_LOOP: u32 = 105;
const EXPR_TYPED_OBJECT_LITERAL: u32 = 102;
const TOKEN_INTEGER: u32 = 3;
const TOKEN_FLOAT: u32 = 4;
const TOKEN_STRING: u32 = 5;
const TOKEN_CHARACTER: u32 = 6;
const TOKEN_EQUAL: u32 = 25;
const TOKEN_BANG: u32 = 26;
const TOKEN_NOT_EQUAL: u32 = 27;
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
const TOKEN_SLASH_ASSIGN: u32 = 47;
const TOKEN_PERCENT: u32 = 48;
const TOKEN_PERCENT_ASSIGN: u32 = 49;
const TOKEN_AMPERSAND: u32 = 50;
const TOKEN_LOGICAL_AND: u32 = 51;
const TOKEN_AMPERSAND_ASSIGN: u32 = 52;
const TOKEN_PIPE: u32 = 53;
const TOKEN_LOGICAL_OR: u32 = 54;
const TOKEN_PIPE_ASSIGN: u32 = 55;
const TOKEN_CARET: u32 = 56;
const TOKEN_CARET_ASSIGN: u32 = 57;
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
const PREFIX_UPDATE: u32 = 1 << 26;
const FOR_INITIALIZER: u32 = 1 << 27;
const FOR_CONDITION: u32 = 1 << 28;
const FOR_UPDATE: u32 = 1 << 29;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum LoweringError
{
  InvalidRoot,
  MissingIdentifier,
  MissingParameterType,
  ModuleMismatch,
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
  if value.kind == IDENTIFIER
  {
    return value.text.clone();
  }
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
    if name == "String"
    {
      return declarations::TypeRef::Named("Optional<Str>".to_string());
    }
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
    declarations::TypeRef::Unit => crate::hir::type_check::Type::Unit,
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

struct LoweringContext
{
  calls: HashMap<String, CallSignature>,
  nominal_types: HashMap<String, declarations::NominalType>,
}

fn lower_expression(tree: &SyntaxTree,
                    value: &SyntaxNode,
                    context: &LoweringContext,
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
    EXPR_LITERAL if value.token_kind == TOKEN_FLOAT => Some(Expression::Literal { literal:
                                                                                    Literal::Float(value.text.clone()),
                                                                                  span: source_span }),
    EXPR_LITERAL if value.token_kind == TOKEN_STRING =>
    {
      let literal = Expression::Literal { literal: Literal::String(value.text.trim_matches('"').to_string()),
                                          span: source_span };
      if expected_type.is_some_and(Type::is_boxed_optional_str)
      {
        return Some(Expression::Call { function: "Some".to_string(),
                                       arguments: vec![literal],
                                       parameter_types: vec![Type::Primitive(PrimitiveType::Str)],
                                       return_type: Box::new(Type::Named("Optional<Str>".to_string())),
                                       span: source_span });
      }
      Some(literal)
    }
    EXPR_LITERAL if value.token_kind == TOKEN_CHARACTER =>
    {
      Some(Expression::Literal { literal: Literal::Char(crate::text_literal::decode_character(&value.text)?),
                                 span: source_span })
    }
    EXPR_LITERAL if value.text == "None" && expected_type.is_some_and(Type::is_boxed_optional_str) =>
    {
      Some(Expression::Literal { literal: Literal::None,
                                 span: source_span })
    }
    EXPR_LITERAL if value.text == "true" || value.text == "false" =>
    {
      Some(Expression::Literal { literal: Literal::Bool(value.text == "true"),
                                 span: source_span })
    }
    EXPR_IDENTIFIER => Some(Expression::Local { name: path_text(tree, value),
                                                span: source_span }),
    EXPR_MEMBER_ACCESS => nominal::lower_field_expression(tree, value, context, locals),
    EXPR_OBJECT_LITERAL | EXPR_TYPED_OBJECT_LITERAL =>
    {
      nominal::lower_object_expression(tree, value, context, locals, expected_type, source_span)
    }
    EXPR_UNARY if value.children.len() == 1 =>
    {
      unary::lower_unary_expression(tree, value, context, locals, expected_type, source_span)
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
        TOKEN_LOGICAL_AND => BinaryOperator::LogicalAnd,
        TOKEN_PIPE => BinaryOperator::BitOr,
        TOKEN_LOGICAL_OR => BinaryOperator::LogicalOr,
        TOKEN_CARET => BinaryOperator::BitXor,
        TOKEN_SHIFT_LEFT => BinaryOperator::ShiftLeft,
        TOKEN_SHIFT_RIGHT => BinaryOperator::ShiftRight,
        TOKEN_EQUAL => BinaryOperator::Equal,
        TOKEN_NOT_EQUAL => BinaryOperator::NotEqual,
        TOKEN_LESS => BinaryOperator::Less,
        TOKEN_LESS_EQUAL => BinaryOperator::LessEqual,
        TOKEN_GREATER => BinaryOperator::Greater,
        TOKEN_GREATER_EQUAL => BinaryOperator::GreaterEqual,
        _ => return None,
      };
      let bool_type = Type::Primitive(PrimitiveType::Bool);
      let operand_type = if matches!(operator, BinaryOperator::LogicalAnd | BinaryOperator::LogicalOr)
      {
        Some(&bool_type)
      }
      else if matches!(operator,
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
      let left = lower_expression(tree, tree.nodes.get(value.children[0])?, context, locals, operand_type)?;
      let right = lower_expression(tree, tree.nodes.get(value.children[2])?, context, locals, operand_type)?;
      Some(Expression::Binary { operator,
                                left: Box::new(left),
                                right: Box::new(right),
                                span: source_span })
    }
    EXPR_ASSIGNMENT if value.children.len() == 3 =>
    {
      let target = tree.nodes.get(value.children[0])?;
      if target.kind == EXPR_MEMBER_ACCESS
      {
        return nominal::lower_field_assignment(tree, value, context, locals, source_span);
      }
      if target.kind != EXPR_IDENTIFIER
      {
        return None;
      }
      let target = path_text(tree, target);
      let assigned = lower_expression(tree,
                                      tree.nodes.get(value.children[2])?,
                                      context,
                                      locals,
                                      locals.get(&target))?;
      let assigned = match value.token_kind
      {
        TOKEN_ASSIGN => assigned,
        TOKEN_PLUS_ASSIGN |
        TOKEN_MINUS_ASSIGN |
        TOKEN_STAR_ASSIGN |
        TOKEN_SLASH_ASSIGN |
        TOKEN_PERCENT_ASSIGN |
        TOKEN_AMPERSAND_ASSIGN |
        TOKEN_PIPE_ASSIGN |
        TOKEN_CARET_ASSIGN =>
        {
          let operator = match value.token_kind
          {
            TOKEN_PLUS_ASSIGN => BinaryOperator::Add,
            TOKEN_MINUS_ASSIGN => BinaryOperator::Sub,
            TOKEN_STAR_ASSIGN => BinaryOperator::Mul,
            TOKEN_SLASH_ASSIGN => BinaryOperator::Div,
            TOKEN_PERCENT_ASSIGN => BinaryOperator::Rem,
            TOKEN_AMPERSAND_ASSIGN => BinaryOperator::BitAnd,
            TOKEN_PIPE_ASSIGN => BinaryOperator::BitOr,
            TOKEN_CARET_ASSIGN => BinaryOperator::BitXor,
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
      if function == "Some" && expected_type.is_some_and(Type::is_boxed_optional_str) && value.children.len() == 2
      {
        let argument = lower_expression(tree,
                                        tree.nodes.get(value.children[1])?,
                                        context,
                                        locals,
                                        Some(&Type::Primitive(PrimitiveType::Str)))?;
        return Some(Expression::Call { function,
                                       arguments: vec![argument],
                                       parameter_types: vec![Type::Primitive(PrimitiveType::Str)],
                                       return_type: Box::new(Type::Named("Optional<Str>".to_string())),
                                       span: source_span });
      }
      let signature = context.calls.get(&function)?;
      if value.children.len() - 1 != signature.parameters.len()
      {
        return None;
      }
      let arguments =
        value.children[1..].iter()
                           .zip(&signature.parameters)
                           .map(|(index, parameter)| {
                             lower_expression(tree, tree.nodes.get(*index)?, context, locals, Some(parameter))
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
                                       context,
                                       locals,
                                       Some(&Type::Primitive(PrimitiveType::Bool)))?;
      let mut then_locals = locals.clone();
      let then_block = lower_hir_block(tree,
                                       tree.nodes.get(value.children[1])?,
                                       context,
                                       &mut then_locals,
                                       None,
                                       Some(&result_type))?;
      let mut else_locals = locals.clone();
      let else_block = lower_hir_block(tree,
                                       tree.nodes.get(value.children[2])?,
                                       context,
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
      match_expression::lower_match_expression(tree, value, context, locals, expected_type?.clone())
    }
    _ => None,
  }
}

fn lower_discarded_expression(tree: &SyntaxTree,
                              value: &SyntaxNode,
                              context: &LoweringContext,
                              locals: &HashMap<String, Type>,
                              expected_type: Option<&Type>)
                              -> Option<Expression>
{
  lower_expression(tree, value, context, locals, expected_type)
}

fn lower_local(tree: &SyntaxTree,
               statement: &SyntaxNode,
               context: &LoweringContext,
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
  let name = first_child_kind(tree, declaration, IDENTIFIER)?;
  let initializer_node = declaration.children
                                    .iter()
                                    .filter_map(|index| tree.nodes.get(*index))
                                    .find(|child| child.kind >= EXPR_IDENTIFIER);
  let checked_type = if declaration.flags & INFERRED_TYPE != 0
  {
    expression_type(tree, initializer_node?, context, locals)?
  }
  else
  {
    let ty = declaration.children
                        .iter()
                        .filter_map(|index| tree.nodes.get(*index))
                        .find(|child| (TYPE_NAMED..=TYPE_UNIT).contains(&child.kind))?;
    checked_type(&lower_type(tree, ty))?
  };
  let initializer = match initializer_node
  {
    Some(value) => Some(lower_expression(tree, value, context, locals, Some(&checked_type))?),
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
                        context: &LoweringContext,
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
        Some(value) => Some(lower_expression(tree, value, context, locals, return_type)?),
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
      lower_discarded_expression(tree, expression, context, locals, expected).map(Statement::Expr)
    }
    STMT_VARIABLE => lower_local(tree, statement, context, locals),
    STMT_IF => lower_if_statement(tree, statement, context, locals, return_type),
    STMT_FOR => lower_for_statement(tree, statement, context, locals, return_type),
    STMT_WHILE => lower_while_statement(tree, statement, context, locals, return_type),
    STMT_LOOP => lower_loop_statement(tree, statement, context, locals, return_type),
    STMT_MATCH => lower_match_statement(tree, statement, context, locals, return_type),
    STMT_BREAK => Some(Statement::Break { span: span(statement)? }),
    STMT_CONTINUE => Some(Statement::Continue { span: span(statement)? }),
    _ => None,
  }
}

fn lower_hir_block(tree: &SyntaxTree,
                   block: &SyntaxNode,
                   context: &LoweringContext,
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
      tail = Some(Box::new(lower_expression(tree, expression, context, locals, tail_type)?));
    }
    else
    {
      statements.push(lower_statement_node(tree, statement, context, locals, return_type)?);
    }
  }
  Some(Block { statements,
               tail,
               span: span(block)? })
}

fn lower_if_branch(tree: &SyntaxTree,
                   branch: &SyntaxNode,
                   context: &LoweringContext,
                   locals: &HashMap<String, Type>,
                   return_type: Option<&Type>,
                   else_block: Option<Block>)
                   -> Option<Block>
{
  let condition = lower_expression(tree,
                                   tree.nodes.get(*branch.children.first()?)?,
                                   context,
                                   locals,
                                   Some(&Type::Primitive(PrimitiveType::Bool)))?;
  let mut branch_locals = locals.clone();
  let then_block = lower_hir_block(tree,
                                   tree.nodes.get(*branch.children.get(1)?)?,
                                   context,
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
                      context: &LoweringContext,
                      locals: &HashMap<String, Type>,
                      return_type: Option<&Type>)
                      -> Option<Statement>
{
  let condition = lower_expression(tree,
                                   tree.nodes.get(*statement.children.first()?)?,
                                   context,
                                   locals,
                                   Some(&Type::Primitive(PrimitiveType::Bool)))?;
  let mut then_locals = locals.clone();
  let then_block = lower_hir_block(tree,
                                   tree.nodes.get(*statement.children.get(1)?)?,
                                   context,
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
      Some(lower_hir_block(tree, block, context, &mut branch_locals, return_type, None)?)
    }
    None => None,
  };
  for branch in statement.children[2..].iter()
                                       .filter_map(|index| tree.nodes.get(*index))
                                       .filter(|node| node.kind == STMT_ELSE_IF)
                                       .rev()
  {
    else_block = Some(lower_if_branch(tree, branch, context, locals, return_type, else_block)?);
  }
  Some(Statement::If { condition,
                       then_block,
                       else_block,
                       span: span(statement)? })
}

fn lower_while_statement(tree: &SyntaxTree,
                         statement: &SyntaxNode,
                         context: &LoweringContext,
                         locals: &HashMap<String, Type>,
                         return_type: Option<&Type>)
                         -> Option<Statement>
{
  let condition = lower_expression(tree,
                                   tree.nodes.get(*statement.children.first()?)?,
                                   context,
                                   locals,
                                   Some(&Type::Primitive(PrimitiveType::Bool)))?;
  let mut body_locals = locals.clone();
  let mut body = lower_hir_block(tree,
                                 tree.nodes.get(*statement.children.get(1)?)?,
                                 context,
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

fn lower_loop_statement(tree: &SyntaxTree,
                        statement: &SyntaxNode,
                        context: &LoweringContext,
                        locals: &HashMap<String, Type>,
                        return_type: Option<&Type>)
                        -> Option<Statement>
{
  let mut body_locals = locals.clone();
  let body = lower_hir_block(tree,
                             tree.nodes.get(*statement.children.first()?)?,
                             context,
                             &mut body_locals,
                             return_type,
                             None)?;
  Some(Statement::While { condition: Expression::Literal { literal: Literal::Bool(true),
                                                           span: span(statement)? },
                          body,
                          span: span(statement)? })
}

fn lower_for_statement(tree: &SyntaxTree,
                       statement: &SyntaxNode,
                       context: &LoweringContext,
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
      lower_local(tree, node, context, &mut for_locals)?
    }
    else
    {
      Statement::Expr(lower_discarded_expression(tree, node, context, &for_locals, None)?)
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
                          context,
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
    Some(lower_discarded_expression(tree, node, context, &for_locals, None)?)
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
  let body = lower_hir_block(tree, body_node, context, &mut body_locals, return_type, None)?;
  Some(Statement::For { initializer,
                        condition,
                        update,
                        body,
                        span: span(statement)? })
}

fn lower_match_statement(tree: &SyntaxTree,
                         statement: &SyntaxNode,
                         context: &LoweringContext,
                         locals: &HashMap<String, Type>,
                         return_type: Option<&Type>)
                         -> Option<Statement>
{
  let selector_node = tree.nodes.get(*statement.children.first()?)?;
  let selector_type = expression_type(tree, selector_node, context, locals)?;
  let selector = lower_expression(tree, selector_node, context, locals, Some(&selector_type))?;
  let arms = statement.children[1..].iter()
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
                                          lower_expression(tree, literal_node, context, locals, Some(&selector_type))?
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
                                                                 context,
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
              context: &LoweringContext,
              parameters: &[declarations::Parameter],
              return_type: Option<&Type>)
              -> Option<Vec<Statement>>
{
  let mut locals = parameters.iter()
                             .filter_map(|parameter| checked_type(&parameter.ty).map(|ty| (parameter.name.clone(), ty)))
                             .collect::<HashMap<_, _>>();
  let block = first_child_kind(tree, function, STMT_BLOCK)?;
  let block = lower_hir_block(tree, block, context, &mut locals, return_type, return_type)?;
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
  program::lower_program(std::slice::from_ref(tree))
}

pub fn lower_program(trees: &[SyntaxTree]) -> Result<declarations::Module, LoweringError>
{
  program::lower_program(trees)
}

#[cfg(test)]
mod tests;
