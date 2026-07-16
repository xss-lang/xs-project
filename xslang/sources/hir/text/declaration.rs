/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::fmt::Write;

use crate::compiler_core::SourceSpan;
use crate::hir::declarations::{Field, NominalKind, NominalType, TupleFieldRef, TypeRef};
use crate::hir::type_check::Type;

use super::names::type_name;
use super::parser::{XhirParseDiagnostic, type_parser::parse_type_text};

pub(super) fn write_declarations(output: &mut String, declarations: &[NominalType])
{
  if declarations.is_empty()
  {
    return;
  }
  output.push_str("\ndeclarations\n");
  for declaration in declarations
  {
    let kind = match declaration.kind
    {
      NominalKind::Class => "class",
      NominalKind::Data => "data",
    };
    let _ = writeln!(output, "  {kind} {}", declaration.name);
    for field in &declaration.fields
    {
      let ty = crate::hir::declarations::type_ref_to_checked(&field.ty).map(|ty| type_name(&ty))
                                                                       .unwrap_or_else(|| "<unsupported>".to_string());
      let mutability = if field.mutable
      {
        "mutable"
      }
      else
      {
        "immutable"
      };
      let _ = writeln!(output, "    field {}: {ty} {mutability}", field.name);
    }
    output.push_str("  .end\n");
  }
  output.push_str(".end\n");
}

pub(super) fn parse_declarations(lines: &[&str],
                                 index: &mut usize,
                                 diagnostics: &mut Vec<XhirParseDiagnostic>)
                                 -> Vec<NominalType>
{
  let mut declarations = Vec::new();
  *index += 1;
  while *index < lines.len()
  {
    let line = lines[*index].trim();
    if line.is_empty()
    {
      *index += 1;
      continue;
    }
    if line == ".end"
    {
      *index += 1;
      return declarations;
    }
    let (kind, name) = if let Some(name) = line.strip_prefix("data ")
    {
      (NominalKind::Data, name)
    }
    else if let Some(name) = line.strip_prefix("class ")
    {
      (NominalKind::Class, name)
    }
    else
    {
      diagnostics.push(error(*index + 1, format!("unexpected XHIR declaration '{line}'")));
      *index += 1;
      continue;
    };
    let start_line = *index + 1;
    *index += 1;
    let fields = parse_fields(lines, index, diagnostics);
    declarations.push(NominalType { name: name.to_string(),
                                    kind,
                                    fields,
                                    span: source_span(start_line) });
  }
  diagnostics.push(error(lines.len().max(1), "unterminated XHIR declarations section".to_string()));
  declarations
}

fn parse_fields(lines: &[&str], index: &mut usize, diagnostics: &mut Vec<XhirParseDiagnostic>) -> Vec<Field>
{
  let mut fields = Vec::new();
  while *index < lines.len()
  {
    let line_number = *index + 1;
    let line = lines[*index].trim();
    if line == ".end"
    {
      *index += 1;
      return fields;
    }
    *index += 1;
    let (record, mutable) = if let Some(record) = line.strip_suffix(" mutable")
    {
      (record, true)
    }
    else if let Some(record) = line.strip_suffix(" immutable")
    {
      (record, false)
    }
    else
    {
      diagnostics.push(error(line_number, "nominal field requires mutability".to_string()));
      continue;
    };
    let Some((name, ty)) = record.strip_prefix("field ").and_then(|record| record.split_once(": "))
    else
    {
      diagnostics.push(error(line_number, "expected nominal field record".to_string()));
      continue;
    };
    let Some(ty) = parse_type_text(ty).and_then(checked_to_ref)
    else
    {
      diagnostics.push(error(line_number, format!("unknown nominal field type '{ty}'")));
      continue;
    };
    fields.push(Field { name: name.to_string(),
                        ty,
                        mutable,
                        span: source_span(line_number) });
  }
  diagnostics.push(error(lines.len().max(1), "unterminated nominal declaration".to_string()));
  fields
}

fn checked_to_ref(ty: Type) -> Option<TypeRef>
{
  Some(match ty
  {
    Type::Unit => TypeRef::Unit,
    Type::Primitive(value) => TypeRef::Primitive(value),
    Type::Named(value) => TypeRef::Named(value),
    Type::Array { element,
                  length, } => TypeRef::Array { element: Box::new(checked_to_ref(*element)?),
                                                length },
    Type::Map { key,
                value, } => TypeRef::Map { key: Box::new(checked_to_ref(*key)?),
                                           value: Box::new(checked_to_ref(*value)?) },
    Type::Tuple { fields } => TypeRef::Tuple { fields: fields.into_iter()
                                                             .map(|field| {
                                                               Some(TupleFieldRef { name: field.name,
                                                                                    ty: checked_to_ref(field.ty)? })
                                                             })
                                                             .collect::<Option<Vec<_>>>()? },
    Type::Set { .. } => return None,
  })
}

const fn source_span(line: usize) -> SourceSpan
{
  SourceSpan { file_id: 0,
               start_offset: 0,
               end_offset: 0,
               start_line: line as u64,
               start_column: 0,
               end_line: line as u64,
               end_column: 0 }
}

fn error(line: usize, message: String) -> XhirParseDiagnostic
{
  XhirParseDiagnostic { line,
                        message }
}
