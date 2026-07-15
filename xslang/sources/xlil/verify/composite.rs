/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::*;

impl Verifier
{
  pub(super) fn composite(&mut self,
                          function: &Function,
                          module: &Module,
                          result: ValueId,
                          composite_type: Type,
                          fields: &[ValueId])
  {
    self.typed_value(function, result, composite_type, "XLIL composite result");
    let (expected, code, label) = match composite_type.kind
    {
      TypeKind::Aggregate =>
      {
        let Some(layout) = module.aggregate_type(composite_type)
        else
        {
          self.report(DiagnosticCode::InvalidAggregateType,
                      "XLIL aggregate instruction references an unknown type");
          return;
        };
        (layout.fields.clone(), DiagnosticCode::InvalidAggregateType, "aggregate")
      }
      TypeKind::Array =>
      {
        let Some(layout) = module.array_type(composite_type)
        else
        {
          self.report(DiagnosticCode::InvalidArrayType,
                      "XLIL array instruction references an unknown type");
          return;
        };
        let Ok(length) = usize::try_from(layout.length)
        else
        {
          self.report(DiagnosticCode::InvalidArrayType,
                      "XLIL array length cannot be represented on this host");
          return;
        };
        (vec![layout.element_type; length], DiagnosticCode::InvalidArrayType, "array")
      }
      _ =>
      {
        self.report(DiagnosticCode::InvalidAggregateType,
                    "XLIL composite instruction requires a registry type");
        return;
      }
    };
    if fields.len() != expected.len()
    {
      self.report(code.clone(),
                  &format!("XLIL {label} value count must match its registry layout"));
    }
    for (field, expected) in fields.iter().zip(expected)
    {
      if value_type(function, *field) != Some(expected)
      {
        self.report(code.clone(),
                    &format!("XLIL {label} value type must match its registry layout"));
      }
    }
  }

  pub(super) fn extract(&mut self,
                        function: &Function,
                        module: &Module,
                        result: ValueId,
                        composite: ValueId,
                        field: u32)
  {
    let Some(composite_type) = value_type(function, composite)
    else
    {
      self.report(DiagnosticCode::InstructionResultUnknown,
                  "XLIL extract source must reference a declared value");
      return;
    };
    let extracted = match composite_type.kind
    {
      TypeKind::Aggregate => module.aggregate_type(composite_type)
                                   .and_then(|layout| layout.fields.get(field as usize))
                                   .copied(),
      TypeKind::Array => module.array_type(composite_type)
                               .filter(|layout| u64::from(field) < layout.length)
                               .map(|layout| layout.element_type),
      _ => None,
    };
    let Some(extracted) = extracted
    else
    {
      self.report(DiagnosticCode::InvalidAggregateType,
                  "XLIL extract index must exist in its composite registry layout");
      return;
    };
    self.typed_value(function, result, extracted, "XLIL extract result");
  }
}
