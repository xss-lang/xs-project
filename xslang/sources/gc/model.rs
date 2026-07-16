/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/// XGC regions have one fixed size in runtime ABI version 0.
pub const REGION_SIZE_BYTES: usize = 2 * 1024 * 1024;

/// Objects at or above half a region use the humongous-object path.
pub const HUMONGOUS_OBJECT_THRESHOLD_BYTES: usize = REGION_SIZE_BYTES / 2;

/// The role currently assigned to one region.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub enum RegionState
{
  #[default]
  Free,
  Eden,
  Survivor,
  Old,
  Humongous,
}

/// Whole-program XGC selection. RAII remains the default mode.
#[derive(Clone, Copy, Debug, Default, Eq, PartialEq)]
pub struct XgcConfiguration
{
  pub enabled: bool,
}

impl XgcConfiguration
{
  #[must_use]
  pub const fn disabled() -> Self
  {
    Self { enabled: false }
  }

  #[must_use]
  pub const fn enabled() -> Self
  {
    Self { enabled: true }
  }

  pub const fn is_humongous(object_size: usize) -> bool
  {
    object_size >= HUMONGOUS_OBJECT_THRESHOLD_BYTES
  }
}

#[cfg(test)]
mod tests
{
  use super::*;

  #[test]
  fn xgc_is_disabled_by_default()
  {
    assert_eq!(XgcConfiguration::default(), XgcConfiguration::disabled());
  }

  #[test]
  fn region_and_humongous_boundaries_are_stable()
  {
    assert_eq!(REGION_SIZE_BYTES, 2_097_152);
    assert!(!XgcConfiguration::is_humongous(HUMONGOUS_OBJECT_THRESHOLD_BYTES - 1));
    assert!(XgcConfiguration::is_humongous(HUMONGOUS_OBJECT_THRESHOLD_BYTES));
  }
}
