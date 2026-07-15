/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::{LocalId, Parser, Statement, Type, span, type_from_name};

pub(super) fn statement(parser: &mut Parser<'_>) -> Statement
{
  let result = required_local(parser, "result local ", "aggregate result");
  let value_type = required_type(parser, "type ", "aggregate type");
  let mut fields = Vec::new();
  let mut field_types = Vec::new();
  while let Some(line) = parser.current()
  {
    let Some(field) = line.strip_prefix("field local ")
    else
    {
      break;
    };
    parser.index += 1;
    let Some((local, field_type)) = field.split_once(" type ")
    else
    {
      parser.report("expected aggregate field local and type".to_string());
      continue;
    };
    fields.push(parser.local_id(local));
    field_types.push(parse_type(parser, field_type, "aggregate field type"));
  }
  Statement::Aggregate { result,
                         value_type,
                         fields,
                         field_types,
                         span: span() }
}

pub(super) fn extract_statement(parser: &mut Parser<'_>) -> Statement
{
  let result = required_local(parser, "result local ", "extract result");
  let aggregate = required_local(parser, "aggregate local ", "extract aggregate");
  let field = required_u32(parser, "field ", "extract field");
  let field_type = required_type(parser, "type ", "extract type");
  Statement::Extract { result,
                       aggregate,
                       field,
                       field_type,
                       span: span() }
}

fn required_local(parser: &mut Parser<'_>, prefix: &str, label: &str) -> LocalId
{
  let Some(line) = parser.current()
  else
  {
    parser.report(format!("missing {label}"));
    return LocalId(0);
  };
  parser.index += 1;
  let Some(value) = line.strip_prefix(prefix)
  else
  {
    parser.report(format!("expected {label}"));
    return LocalId(0);
  };
  parser.local_id(value)
}

fn required_u32(parser: &mut Parser<'_>, prefix: &str, label: &str) -> u32
{
  let Some(line) = parser.current()
  else
  {
    parser.report(format!("missing {label}"));
    return 0;
  };
  parser.index += 1;
  let Some(value) = line.strip_prefix(prefix).and_then(|value| value.parse().ok())
  else
  {
    parser.report(format!("expected {label}"));
    return 0;
  };
  value
}

fn required_type(parser: &mut Parser<'_>, prefix: &str, label: &str) -> Type
{
  let Some(line) = parser.current()
  else
  {
    parser.report(format!("missing {label}"));
    return Type::VOID;
  };
  parser.index += 1;
  let Some(value) = line.strip_prefix(prefix)
  else
  {
    parser.report(format!("expected {label}"));
    return Type::VOID;
  };
  parse_type(parser, value, label)
}

fn parse_type(parser: &mut Parser<'_>, value: &str, label: &str) -> Type
{
  if let Some(id) = value.strip_prefix("%t").and_then(|id| id.parse().ok())
  {
    return Type::aggregate(id);
  }
  if let Some(id) = value.strip_prefix("%a").and_then(|id| id.parse().ok())
  {
    return Type::array(id);
  }
  type_from_name(value).unwrap_or_else(|| {
                         parser.report(format!("unknown {label} '{value}'"));
                         Type::VOID
                       })
}
