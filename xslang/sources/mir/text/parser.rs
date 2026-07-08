/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use crate::hir::async_check::Span;
use crate::mir::{BasicBlock, BlockId, Function, Local, LocalId, Statement, Terminator};

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct XmirParseDiagnostic
{
  pub line: usize,
  pub message: String,
}

#[must_use]
pub fn parse_xmir_function(text: &str) -> Result<Function, Vec<XmirParseDiagnostic>>
{
  let mut parser = Parser { lines: text.lines().collect(),
                            index: 0,
                            diagnostics: Vec::new() };
  let function = parser.function();
  if parser.diagnostics.is_empty()
  {
    Ok(function)
  }
  else
  {
    Err(parser.diagnostics)
  }
}

struct Parser<'a>
{
  lines: Vec<&'a str>,
  index: usize,
  diagnostics: Vec<XmirParseDiagnostic>,
}

impl Parser<'_>
{
  fn function(&mut self) -> Function
  {
    self.expect_exact(".xmir version 0");
    let name = self.function_name();
    let mut function = Function { name,
                                  locals: Vec::new(),
                                  blocks: Vec::new() };
    while let Some(line) = self.next_non_empty()
    {
      match line.as_str()
      {
        "locals" => self.locals(&mut function),
        "control_flow" => self.control_flow(&mut function),
        other =>
        {
          self.report(format!("unexpected XMIR section '{other}'"));
          self.index += 1;
        }
      }
    }
    function
  }

  fn function_name(&mut self) -> String
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing function declaration".to_string());
      return String::new();
    };
    let Some(name) = line.strip_prefix("function ")
    else
    {
      self.report("expected function declaration".to_string());
      return String::new();
    };
    self.index += 1;
    name.to_string()
  }

  fn locals(&mut self, function: &mut Function)
  {
    self.index += 1;
    while let Some(line) = self.current()
    {
      if line.is_empty()
      {
        self.index += 1;
        continue;
      }
      let Some(id_text) = line.strip_prefix("  local ")
      else
      {
        break;
      };
      let id = self.local_id(id_text);
      self.index += 1;
      let name = self.local_name();
      let mutable = self.local_mutability();
      function.locals.push(Local { id,
                                   name,
                                   mutable,
                                   span: span() });
    }
  }

  fn control_flow(&mut self, function: &mut Function)
  {
    self.index += 1;
    while let Some(line) = self.current()
    {
      if line.is_empty()
      {
        self.index += 1;
        continue;
      }
      let Some(id_text) = line.strip_prefix("  block ")
      else
      {
        break;
      };
      let id = self.block_id(id_text);
      self.index += 1;
      function.blocks.push(self.block(id));
    }
  }

  fn block(&mut self, id: BlockId) -> BasicBlock
  {
    let mut block = BasicBlock { id,
                                 statements: Vec::new(),
                                 terminator: None,
                                 span: span() };
    if self.current().as_deref() == Some("    statements")
    {
      self.index += 1;
      self.statements(&mut block);
    }
    block.terminator = self.terminator();
    block
  }

  fn statements(&mut self, block: &mut BasicBlock)
  {
    while let Some(line) = self.current()
    {
      let Some(kind) = line.strip_prefix("      statement ")
      else
      {
        break;
      };
      self.index += 1;
      let local = self.statement_local();
      match kind
      {
        "use" => block.statements.push(Statement::Use { local,
                                                        span: span() }),
        "move" => block.statements.push(Statement::Move { local,
                                                          span: span() }),
        "borrow shared" => block.statements.push(Statement::BorrowShared { local,
                                                                           span: span() }),
        "borrow mutable" => block.statements.push(Statement::BorrowMutable { local,
                                                                             span: span() }),
        "borrow end" => block.statements.push(Statement::EndBorrow { local,
                                                                     span: span() }),
        "drop" => block.statements.push(Statement::Drop { local,
                                                          span: span() }),
        _ => self.report(format!("unknown statement kind '{kind}'")),
      }
    }
  }

  fn terminator(&mut self) -> Option<Terminator>
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing block terminator".to_string());
      return None;
    };
    let Some(kind) = line.strip_prefix("    terminator ")
    else
    {
      self.report("expected block terminator".to_string());
      return None;
    };
    self.index += 1;
    match kind
    {
      "missing" => None,
      "return" => Some(self.return_terminator()),
      "goto" => Some(Terminator::Goto(self.goto_target())),
      "unreachable" => Some(Terminator::Unreachable),
      _ =>
      {
        self.report(format!("unknown terminator '{kind}'"));
        None
      }
    }
  }

  fn return_terminator(&mut self) -> Terminator
  {
    if let Some(line) = self.current() &&
       let Some(local) = line.strip_prefix("      value local ")
    {
      self.index += 1;
      return Terminator::Return(Some(self.local_id(local)));
    }
    Terminator::Return(None)
  }

  fn goto_target(&mut self) -> BlockId
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing goto target".to_string());
      return BlockId(0);
    };
    self.index += 1;
    let Some(target) = line.strip_prefix("      target block ")
    else
    {
      self.report("expected goto target".to_string());
      return BlockId(0);
    };
    self.block_id(target)
  }

  fn local_name(&mut self) -> String
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing local name".to_string());
      return String::new();
    };
    self.index += 1;
    match line.strip_prefix("    name ")
    {
      Some(name) => name.to_string(),
      None =>
      {
        self.report("expected local name".to_string());
        String::new()
      }
    }
  }

  fn local_mutability(&mut self) -> bool
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing local mutability".to_string());
      return false;
    };
    self.index += 1;
    match line.strip_prefix("    mutability ")
    {
      Some("mutable") => true,
      Some("immutable") => false,
      Some(value) =>
      {
        self.report(format!("unknown mutability '{value}'"));
        false
      }
      None =>
      {
        self.report("expected local mutability".to_string());
        false
      }
    }
  }

  fn statement_local(&mut self) -> LocalId
  {
    let Some(line) = self.current()
    else
    {
      self.report("missing statement local".to_string());
      return LocalId(0);
    };
    self.index += 1;
    let Some(local) = line.strip_prefix("        local ")
    else
    {
      self.report("expected statement local".to_string());
      return LocalId(0);
    };
    self.local_id(local)
  }

  fn local_id(&mut self, text: &str) -> LocalId
  {
    match text.parse()
    {
      Ok(id) => LocalId(id),
      Err(_) =>
      {
        self.report(format!("invalid local id '{text}'"));
        LocalId(0)
      }
    }
  }

  fn block_id(&mut self, text: &str) -> BlockId
  {
    match text.parse()
    {
      Ok(id) => BlockId(id),
      Err(_) =>
      {
        self.report(format!("invalid block id '{text}'"));
        BlockId(0)
      }
    }
  }

  fn expect_exact(&mut self, expected: &str)
  {
    if self.current().as_deref() != Some(expected)
    {
      self.report(format!("expected '{expected}'"));
    }
    self.index += 1;
  }

  fn next_non_empty(&mut self) -> Option<String>
  {
    while self.current().as_deref() == Some("")
    {
      self.index += 1;
    }
    self.current()
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

const fn span() -> Span
{
  Span::new(0, 0, 0)
}
