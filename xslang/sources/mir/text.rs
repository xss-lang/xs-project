/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use super::optimizer::{OptimizationPass, OptimizationReport};
use super::verify::{Diagnostic as VerifyDiagnostic, DiagnosticCode as VerifyDiagnosticCode};
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

#[must_use]
pub fn optimizer_analysis_to_xmir(reports: &[OptimizationReport]) -> String
{
  let mut output = String::new();
  let _ = writeln!(output, "analysis optimizer");
  for report in reports
  {
    let _ = writeln!(output, "  pass {}", optimization_pass_name(report.pass));
    let _ = writeln!(output, "    removed_items {}", report.removed_items);
  }
  output
}

pub fn parse_xmir_optimizer_analysis(text: &str) -> Result<Vec<OptimizationReport>, Vec<XmirParseDiagnostic>>
{
  let mut lines = text.lines().enumerate();
  let Some((_, "analysis optimizer")) = lines.next()
  else
  {
    let message = "expected optimizer analysis section".to_string();
    return Err(vec![XmirParseDiagnostic { line: 1,
                                          message }]);
  };
  let mut reports = Vec::new();
  let mut diagnostics = Vec::new();
  while let Some((line_index, line)) = lines.next()
  {
    let Some(pass_name) = line.strip_prefix("  pass ")
    else
    {
      diagnostics.push(XmirParseDiagnostic { line: line_index + 1,
                                             message: format!("expected optimizer pass record, found '{line}'") });
      continue;
    };
    let pass = parse_optimization_pass(pass_name, line_index + 1, &mut diagnostics);
    let Some((removed_line_index, removed_line)) = lines.next()
    else
    {
      diagnostics.push(XmirParseDiagnostic { line: line_index + 1,
                                             message: "missing removed_items record".to_string() });
      break;
    };
    let removed_items = parse_removed_items(removed_line, removed_line_index + 1, &mut diagnostics);
    if let (Some(pass), Some(removed_items)) = (pass, removed_items)
    {
      reports.push(OptimizationReport { pass,
                                        removed_items });
    }
  }
  if diagnostics.is_empty()
  {
    Ok(reports)
  }
  else
  {
    Err(diagnostics)
  }
}

#[must_use]
pub fn verify_analysis_to_xmir(diagnostics: &[VerifyDiagnostic]) -> String
{
  let mut output = String::new();
  let _ = writeln!(output, "analysis verify");
  for diagnostic in diagnostics
  {
    let _ = writeln!(output, "  diagnostic {}", verify_diagnostic_code_name(&diagnostic.code));
    let _ = writeln!(output,
                     "    span {}:{}..{}",
                     diagnostic.span.file_id, diagnostic.span.start, diagnostic.span.end);
    let _ = writeln!(output, "    message {}", diagnostic.message);
  }
  output
}

pub fn parse_xmir_verify_analysis(text: &str) -> Result<Vec<VerifyDiagnostic>, Vec<XmirParseDiagnostic>>
{
  let mut parser = VerifyAnalysisParser { lines: text.lines().collect(),
                                          index: 0,
                                          diagnostics: Vec::new() };
  let diagnostics = parser.parse();
  if parser.diagnostics.is_empty()
  {
    Ok(diagnostics)
  }
  else
  {
    Err(parser.diagnostics)
  }
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

fn verify_diagnostic_code_name(code: &VerifyDiagnosticCode) -> &'static str
{
  match code
  {
    VerifyDiagnosticCode::DuplicateLocal => "duplicate_local",
    VerifyDiagnosticCode::DuplicateBlock => "duplicate_block",
    VerifyDiagnosticCode::MissingTerminator => "missing_terminator",
    VerifyDiagnosticCode::UnknownLocal => "unknown_local",
    VerifyDiagnosticCode::UnknownBlock => "unknown_block",
  }
}

fn parse_verify_diagnostic_code(name: &str) -> Option<VerifyDiagnosticCode>
{
  match name
  {
    "duplicate_local" => Some(VerifyDiagnosticCode::DuplicateLocal),
    "duplicate_block" => Some(VerifyDiagnosticCode::DuplicateBlock),
    "missing_terminator" => Some(VerifyDiagnosticCode::MissingTerminator),
    "unknown_local" => Some(VerifyDiagnosticCode::UnknownLocal),
    "unknown_block" => Some(VerifyDiagnosticCode::UnknownBlock),
    _ => None,
  }
}

const fn optimization_pass_name(pass: OptimizationPass) -> &'static str
{
  match pass
  {
    OptimizationPass::RemoveUnreachableBlocks => "remove_unreachable_blocks",
    OptimizationPass::RemoveRedundantEndBorrow => "remove_redundant_end_borrow",
  }
}

struct VerifyAnalysisParser<'a>
{
  lines: Vec<&'a str>,
  index: usize,
  diagnostics: Vec<XmirParseDiagnostic>,
}

impl VerifyAnalysisParser<'_>
{
  fn parse(&mut self) -> Vec<VerifyDiagnostic>
  {
    if self.current().as_deref() != Some("analysis verify")
    {
      self.report("expected verify analysis section".to_string());
      return Vec::new();
    }
    self.index += 1;
    let mut parsed = Vec::new();
    while let Some(line) = self.current()
    {
      let Some(code_name) = line.strip_prefix("  diagnostic ").map(ToString::to_string)
      else
      {
        self.report(format!("expected verifier diagnostic record, found '{line}'"));
        self.index += 1;
        continue;
      };
      let code = self.diagnostic_code(&code_name);
      self.index += 1;
      let span = self.span();
      let message = self.message();
      if let (Some(code), Some(span), Some(message)) = (code, span, message)
      {
        parsed.push(VerifyDiagnostic { code,
                                       message,
                                       span });
      }
    }
    parsed
  }

  fn diagnostic_code(&mut self, name: &str) -> Option<VerifyDiagnosticCode>
  {
    let code = parse_verify_diagnostic_code(name);
    if code.is_none()
    {
      self.report(format!("unknown verifier diagnostic code '{name}'"));
    }
    code
  }

  fn span(&mut self) -> Option<crate::hir::async_check::Span>
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing diagnostic span".to_string());
      return None;
    };
    self.index += 1;
    let Some(text) = line.strip_prefix("    span ").map(ToString::to_string)
    else
    {
      self.report("expected diagnostic span".to_string());
      return None;
    };
    parse_span(&text).or_else(|| {
                       self.report(format!("invalid diagnostic span '{text}'"));
                       None
                     })
  }

  fn message(&mut self) -> Option<String>
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing diagnostic message".to_string());
      return None;
    };
    self.index += 1;
    let Some(message) = line.strip_prefix("    message ").map(ToString::to_string)
    else
    {
      self.report("expected diagnostic message".to_string());
      return None;
    };
    Some(message)
  }

  fn current(&self) -> Option<String>
  {
    self.lines.get(self.index).map(|line| (*line).to_string())
  }

  fn report(&mut self, message: String)
  {
    self.diagnostics.push(XmirParseDiagnostic { line: self.index + 1,
                                                message });
  }
}

fn parse_span(text: &str) -> Option<crate::hir::async_check::Span>
{
  let (file_id, rest) = text.split_once(':')?;
  let (start, end) = rest.split_once("..")?;
  Some(crate::hir::async_check::Span::new(file_id.parse().ok()?, start.parse().ok()?, end.parse().ok()?))
}

fn parse_optimization_pass(name: &str,
                           line: usize,
                           diagnostics: &mut Vec<XmirParseDiagnostic>)
                           -> Option<OptimizationPass>
{
  match name
  {
    "remove_unreachable_blocks" => Some(OptimizationPass::RemoveUnreachableBlocks),
    "remove_redundant_end_borrow" => Some(OptimizationPass::RemoveRedundantEndBorrow),
    _ =>
    {
      diagnostics.push(XmirParseDiagnostic { line,
                                             message: format!("unknown optimizer pass '{name}'") });
      None
    }
  }
}

fn parse_removed_items(line: &str, line_number: usize, diagnostics: &mut Vec<XmirParseDiagnostic>) -> Option<usize>
{
  let Some(value) = line.strip_prefix("    removed_items ")
  else
  {
    diagnostics.push(XmirParseDiagnostic { line: line_number,
                                           message: "expected removed_items record".to_string() });
    return None;
  };
  match value.parse()
  {
    Ok(value) => Some(value),
    Err(_) =>
    {
      diagnostics.push(XmirParseDiagnostic { line: line_number,
                                             message: format!("invalid removed_items value '{value}'") });
      None
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

  #[test]
  fn roundtrips_optimizer_analysis()
  {
    let reports = vec![OptimizationReport { pass: OptimizationPass::RemoveUnreachableBlocks,
                                            removed_items: 2 },
                       OptimizationReport { pass: OptimizationPass::RemoveRedundantEndBorrow,
                                            removed_items: 1 }];

    let text = optimizer_analysis_to_xmir(&reports);
    let parsed = parse_xmir_optimizer_analysis(&text).expect("optimizer analysis should parse");

    assert_eq!(parsed, reports);
    assert!(text.contains("analysis optimizer"));
    assert!(!text.contains(".pass"));
  }

  #[test]
  fn rejects_invalid_optimizer_analysis()
  {
    let diagnostics = parse_xmir_optimizer_analysis("analysis optimizer\n  pass nope\n    removed_items many\n")
      .expect_err("invalid optimizer analysis must fail");

    assert_eq!(diagnostics.len(), 2);
  }

  #[test]
  fn roundtrips_verify_analysis()
  {
    let diagnostics = vec![VerifyDiagnostic { code: VerifyDiagnosticCode::UnknownLocal,
                                              message: "local id 7 is not declared".to_string(),
                                              span: span() },
                           VerifyDiagnostic { code: VerifyDiagnosticCode::MissingTerminator,
                                              message: "MIR block is missing a terminator".to_string(),
                                              span: Span::new(1, 4, 8) }];

    let text = verify_analysis_to_xmir(&diagnostics);
    let parsed = parse_xmir_verify_analysis(&text).expect("verify analysis should parse");

    assert_eq!(parsed, diagnostics);
    assert!(text.contains("analysis verify"));
    assert!(!text.contains(".diagnostic"));
  }

  #[test]
  fn rejects_invalid_verify_analysis()
  {
    let diagnostics = parse_xmir_verify_analysis("analysis verify\n  diagnostic nope\n    span bad\n    message \
                                                  broken\n").expect_err("invalid verify analysis must fail");

    assert_eq!(diagnostics.len(), 2);
  }
}
