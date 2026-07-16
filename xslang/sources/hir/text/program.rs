/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use crate::hir::type_check::Function;

use super::parser::{XhirParseDiagnostic, parse_xhir_function};
use super::{SUPPORTED_XHIR_VERSION, function_to_xhir};

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct XhirProgram
{
  pub name: String,
  pub functions: Vec<Function>,
}

#[must_use]
pub fn program_to_xhir(name: &str, functions: &[Function]) -> String
{
  let mut output = format!(".xhir version {SUPPORTED_XHIR_VERSION}\nprogram {name}\n");
  for function in functions
  {
    let document = function_to_xhir(function);
    let body = document.strip_prefix(&format!(".xhir version {SUPPORTED_XHIR_VERSION}\n"))
                       .and_then(|text| text.strip_suffix(".program end\n"))
                       .expect("canonical XHIR function markers");
    output.push('\n');
    output.push_str(body);
    output.push_str(".function end\n");
  }
  output.push_str(".program end\n");
  output
}

pub fn parse_xhir_program(text: &str) -> Result<XhirProgram, Vec<XhirParseDiagnostic>>
{
  let lines = text.lines().collect::<Vec<_>>();
  let mut diagnostics = Vec::new();
  validate_header(&lines, &mut diagnostics);
  let name = program_name(&lines, &mut diagnostics);
  let mut functions = Vec::new();
  let mut index = 2;
  let mut ended = false;
  while index < lines.len()
  {
    let line = lines[index].trim();
    if line.is_empty()
    {
      index += 1;
      continue;
    }
    if line == ".program end"
    {
      ended = true;
      index += 1;
      break;
    }
    if !line.starts_with("function ")
    {
      diagnostics.push(diagnostic(index + 1, format!("unexpected XHIR program record '{line}'")));
      index += 1;
      continue;
    }
    let start = index;
    while index < lines.len() && lines[index].trim() != ".function end"
    {
      index += 1;
    }
    if index == lines.len()
    {
      diagnostics.push(diagnostic(start + 1, "unterminated XHIR function record".to_string()));
      break;
    }
    let document = function_document(&lines[start..index]);
    match parse_xhir_function(&document)
    {
      Ok(function) => functions.push(function),
      Err(errors) => diagnostics.extend(errors.into_iter()
                                              .map(|error| diagnostic(start + error.line, error.message))),
    }
    index += 1;
  }
  if !ended
  {
    diagnostics.push(diagnostic(lines.len().max(1), "missing XHIR program end".to_string()));
  }
  if index < lines.len() && lines[index..].iter().any(|line| !line.trim().is_empty())
  {
    diagnostics.push(diagnostic(index + 1, "records follow XHIR program end".to_string()));
  }
  if diagnostics.is_empty()
  {
    Ok(XhirProgram { name,
                     functions })
  }
  else
  {
    Err(diagnostics)
  }
}

fn function_document(lines: &[&str]) -> String
{
  let mut document = format!(".xhir version {SUPPORTED_XHIR_VERSION}\n");
  for line in lines
  {
    let _ = writeln!(document, "{line}");
  }
  document.push_str(".program end\n");
  document
}

fn validate_header(lines: &[&str], diagnostics: &mut Vec<XhirParseDiagnostic>)
{
  let expected = format!(".xhir version {SUPPORTED_XHIR_VERSION}");
  if lines.first().map(|line| line.trim()) != Some(expected.as_str())
  {
    diagnostics.push(diagnostic(1, format!("expected '{expected}'")));
  }
}

fn program_name(lines: &[&str], diagnostics: &mut Vec<XhirParseDiagnostic>) -> String
{
  if let Some(name) = lines.get(1)
                           .and_then(|line| line.trim().strip_prefix("program "))
                           .filter(|name| !name.is_empty())
  {
    return name.to_string();
  }
  diagnostics.push(diagnostic(2, "expected XHIR program name".to_string()));
  String::new()
}

fn diagnostic(line: usize, message: String) -> XhirParseDiagnostic
{
  XhirParseDiagnostic { line,
                        message }
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::hir::{Function, PrimitiveType, Span, Statement, Type};

  #[test]
  fn roundtrips_multiple_xhir_functions_in_one_program()
  {
    let functions = [Function { name: "first".to_string(),
                                return_type: None,
                                locals: Vec::new(),
                                body: vec![Statement::Return { value: None,
                                                               span: Span::new(0, 0, 1) }] },
                     Function { name: "second".to_string(),
                                return_type: Some(Type::Primitive(PrimitiveType::Long)),
                                locals: Vec::new(),
                                body: Vec::new() }];
    let text = program_to_xhir("root", &functions);
    let parsed = parse_xhir_program(&text).expect("program should parse");
    assert_eq!(parsed.name, "root");
    assert_eq!(program_to_xhir(&parsed.name, &parsed.functions), text);
    assert!(parse_xhir_program(text.trim_end_matches(".program end\n")).is_err());
  }
}
