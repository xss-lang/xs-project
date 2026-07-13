/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::{declarations, type_check::PrimitiveType};

use super::{SyntaxNode, SyntaxTree};

const FILE: u32 = 0;
const DECL_MODULE: u32 = 1;
const DECL_FUNCTION: u32 = 4;
const PARAMETER: u32 = 21;
const IDENTIFIER: u32 = 24;
const PATH: u32 = 25;
const TYPE_NAMED: u32 = 27;
const TYPE_UNIT: u32 = 36;
const RETURN_TYPE: u32 = 1 << 11;

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

fn lower_function(tree: &SyntaxTree, value: &SyntaxNode) -> Result<declarations::Function, LoweringError>
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
                              span: value.span.clone() })
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
  let functions = root.children
                      .iter()
                      .filter_map(|index| tree.nodes.get(*index))
                      .filter(|child| child.kind == DECL_FUNCTION)
                      .map(|function| lower_function(tree, function))
                      .collect::<Result<_, _>>()?;
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
}
