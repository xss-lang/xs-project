/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use super::{BasicBlock, Function, LocalId, Statement, Terminator};

pub mod parser;

pub use parser::{XmirParseDiagnostic, parse_xmir_function};

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct XmirDocumentHeader
{
  pub version: u32,
  pub function: String,
}

#[must_use]
pub fn parse_xmir_header(text: &str) -> Option<XmirDocumentHeader>
{
  let mut lines = text.lines();
  let version = lines.next()?.strip_prefix("xmir version ")?.parse().ok()?;
  let function = lines.next()?.strip_prefix("function ")?.to_string();
  Some(XmirDocumentHeader { version,
                            function })
}

#[must_use]
pub fn function_to_xmir(function: &Function) -> String
{
  let mut output = String::new();
  let _ = writeln!(output, "xmir version 0");
  let _ = writeln!(output, "function {}", function.name);
  if !function.locals.is_empty()
  {
    let _ = writeln!(output);
    let _ = writeln!(output, "locals");
    for local in &function.locals
    {
      let mutability = if local.mutable
      {
        "mutable"
      }
      else
      {
        "immutable"
      };
      let _ = writeln!(output, "  local {}", local.id.0);
      let _ = writeln!(output, "    name {}", local.name);
      let _ = writeln!(output, "    mutability {mutability}");
    }
  }
  if !function.blocks.is_empty()
  {
    let _ = writeln!(output);
    let _ = writeln!(output, "control_flow");
    for block in &function.blocks
    {
      write_block(&mut output, block);
    }
  }
  output
}

fn write_block(output: &mut String, block: &BasicBlock)
{
  let _ = writeln!(output, "  block {}", block.id.0);
  if !block.statements.is_empty()
  {
    let _ = writeln!(output, "    statements");
    for statement in &block.statements
    {
      write_statement(output, statement);
    }
  }
  match &block.terminator
  {
    Some(terminator) => write_terminator(output, terminator),
    None =>
    {
      let _ = writeln!(output, "    terminator missing");
    }
  }
}

fn write_statement(output: &mut String, statement: &Statement)
{
  match statement
  {
    Statement::Use { local, .. } => write_local_statement(output, "use", *local),
    Statement::Move { local, .. } => write_local_statement(output, "move", *local),
    Statement::BorrowShared { local, .. } => write_local_statement(output, "borrow shared", *local),
    Statement::BorrowMutable { local, .. } => write_local_statement(output, "borrow mutable", *local),
    Statement::EndBorrow { local, .. } => write_local_statement(output, "borrow end", *local),
    Statement::Drop { local, .. } => write_local_statement(output, "drop", *local),
  }
}

fn write_local_statement(output: &mut String, name: &str, local: LocalId)
{
  let _ = writeln!(output, "      statement {name}");
  let _ = writeln!(output, "        local {}", local.0);
}

fn write_terminator(output: &mut String, terminator: &Terminator)
{
  match terminator
  {
    Terminator::Return(Some(local)) =>
    {
      let _ = writeln!(output, "    terminator return");
      let _ = writeln!(output, "      value local {}", local.0);
    }
    Terminator::Return(None) =>
    {
      let _ = writeln!(output, "    terminator return");
    }
    Terminator::Goto(block) =>
    {
      let _ = writeln!(output, "    terminator goto");
      let _ = writeln!(output, "      target block {}", block.0);
    }
    Terminator::Unreachable =>
    {
      let _ = writeln!(output, "    terminator unreachable");
    }
  }
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::hir::async_check::Span;
  use crate::mir::{BasicBlock, BlockId, Local, LocalId};

  fn span() -> Span
  {
    Span::new(1, 0, 1)
  }

  #[test]
  fn writes_function_as_structured_xmir()
  {
    let function =
      Function { name: "Main".to_string(),
                 locals: vec![Local { id: LocalId(0),
                                      name: "message".to_string(),
                                      mutable: false,
                                      span: span() }],
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::BorrowShared { local: LocalId(0),
                                                                                      span: span() },
                                                            Statement::EndBorrow { local: LocalId(0),
                                                                                   span: span() },],
                                           terminator: Some(Terminator::Return(None)),
                                           span: span() }] };

    let text = function_to_xmir(&function);

    assert!(text.contains("xmir version 0\nfunction Main"));
    assert!(text.contains("control_flow"));
    assert!(text.contains("statement borrow shared"));
    assert!(text.contains("terminator return"));
    assert!(!text.contains(".func"));
    assert!(!text.contains("%0"));

    let header = parse_xmir_header(&text).expect("header should parse");
    assert_eq!(header.function, "Main");

    let parsed = parse_xmir_function(&text).expect("XMIR function should parse");
    assert_eq!(parsed.name, "Main");
    assert_eq!(parsed.locals.len(), 1);
    assert_eq!(parsed.blocks.len(), 1);
    assert_eq!(parsed.blocks[0].statements.len(), 2);
    assert_eq!(parsed.blocks[0].terminator, Some(Terminator::Return(None)));
  }

  #[test]
  fn roundtrips_goto_return_value_and_unreachable()
  {
    let function =
      Function { name: "Flow".to_string(),
                 locals: vec![Local { id: LocalId(0),
                                      name: "result".to_string(),
                                      mutable: true,
                                      span: span() }],
                 blocks: vec![BasicBlock { id: BlockId(0),
                                           statements: vec![Statement::Use { local: LocalId(0),
                                                                             span: span() },
                                                            Statement::Move { local: LocalId(0),
                                                                              span: span() },
                                                            Statement::Drop { local: LocalId(0),
                                                                              span: span() }],
                                           terminator: Some(Terminator::Goto(BlockId(1))),
                                           span: span() },
                              BasicBlock { id: BlockId(1),
                                           statements: vec![Statement::BorrowMutable { local: LocalId(0),
                                                                                       span: span() }],
                                           terminator: Some(Terminator::Return(Some(LocalId(0)))),
                                           span: span() },
                              BasicBlock { id: BlockId(2),
                                           statements: vec![],
                                           terminator: Some(Terminator::Unreachable),
                                           span: span() }] };

    let text = function_to_xmir(&function);
    let parsed = parse_xmir_function(&text).expect("XMIR function should parse");

    assert_eq!(parsed.name, "Flow");
    assert_eq!(parsed.locals[0].name, "result");
    assert!(parsed.locals[0].mutable);
    assert_eq!(parsed.blocks[0].terminator, Some(Terminator::Goto(BlockId(1))));
    assert_eq!(parsed.blocks[1].terminator, Some(Terminator::Return(Some(LocalId(0)))));
    assert_eq!(parsed.blocks[2].terminator, Some(Terminator::Unreachable));
    assert!(matches!(parsed.blocks[0].statements[0], Statement::Use { local: LocalId(0),
                                                                      .. }));
    assert!(matches!(parsed.blocks[0].statements[1], Statement::Move { local: LocalId(0),
                                                                       .. }));
    assert!(matches!(parsed.blocks[0].statements[2], Statement::Drop { local: LocalId(0),
                                                                       .. }));
    assert!(matches!(parsed.blocks[1].statements[0], Statement::BorrowMutable { local:
                                                                                  LocalId(0),
                                                                                .. }));
  }

  #[test]
  fn rejects_non_xmir_header()
  {
    assert_eq!(parse_xmir_header(".func Main : () -> void\n"), None);
  }

  #[test]
  fn rejects_invalid_xmir_function()
  {
    let diagnostics = parse_xmir_function("xmir version 0\nfunction Main\n\ncontrol_flow\n  block nope\n    \
                                           terminator return\n").expect_err("invalid block id should fail");

    assert_eq!(diagnostics.len(), 1);
  }

  #[test]
  fn parsed_xmir_can_be_structurally_verified()
  {
    let function = Function { name: "Verified".to_string(),
                              locals: vec![Local { id: LocalId(0),
                                                   name: "value".to_string(),
                                                   mutable: false,
                                                   span: span() }],
                              blocks: vec![BasicBlock { id: BlockId(0),
                                                        statements: vec![Statement::Use { local: LocalId(0),
                                                                                          span: span() }],
                                                        terminator: Some(Terminator::Return(Some(LocalId(0)))),
                                                        span: span() }] };
    let text = function_to_xmir(&function);
    let parsed = parse_xmir_function(&text).expect("XMIR function should parse");

    assert!(crate::mir::verify::verify_function(&parsed).is_empty());
  }
}
