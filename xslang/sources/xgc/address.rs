/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

use super::{REGION_SIZE_BYTES, RegionId};

pub const OBJECT_ALIGNMENT_BYTES: u64 = 8;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum GcAddressError
{
  Misaligned,
  OutsideRegion,
  ArithmeticOverflow,
  RegionIdOverflow,
}

/// A target-independent byte offset from the beginning of the managed heap.
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct HeapOffset(u64);

impl HeapOffset
{
  pub const ZERO: Self = Self(0);

  #[must_use]
  pub const fn bytes(self) -> u64
  {
    self.0
  }

  pub fn from_region(region: RegionId, region_offset: u64) -> Result<Self, GcAddressError>
  {
    if region_offset >= REGION_SIZE_BYTES as u64
    {
      return Err(GcAddressError::OutsideRegion);
    }
    let base = u64::from(region.0).checked_mul(REGION_SIZE_BYTES as u64)
                                  .ok_or(GcAddressError::ArithmeticOverflow)?;
    base.checked_add(region_offset)
        .map(Self)
        .ok_or(GcAddressError::ArithmeticOverflow)
  }

  pub fn region(self) -> Result<RegionId, GcAddressError>
  {
    let index = self.0 / REGION_SIZE_BYTES as u64;
    u32::try_from(index).map(RegionId)
                        .map_err(|_| GcAddressError::RegionIdOverflow)
  }

  #[must_use]
  pub const fn region_offset(self) -> u64
  {
    self.0 % REGION_SIZE_BYTES as u64
  }
}

/// A precise managed reference. It is not a stable native address.
#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct ObjectReference(HeapOffset);

impl ObjectReference
{
  pub fn new(offset: HeapOffset) -> Result<Self, GcAddressError>
  {
    if !offset.bytes().is_multiple_of(OBJECT_ALIGNMENT_BYTES)
    {
      return Err(GcAddressError::Misaligned);
    }
    Ok(Self(offset))
  }

  pub fn from_region(region: RegionId, region_offset: u64) -> Result<Self, GcAddressError>
  {
    Self::new(HeapOffset::from_region(region, region_offset)?)
  }

  #[must_use]
  pub const fn offset(self) -> HeapOffset
  {
    self.0
  }

  pub fn region(self) -> Result<RegionId, GcAddressError>
  {
    self.0.region()
  }

  #[must_use]
  pub const fn region_offset(self) -> u64
  {
    self.0.region_offset()
  }
}

#[cfg(test)]
mod tests
{
  use super::*;

  #[test]
  fn reference_roundtrips_region_and_offset()
  {
    let reference = ObjectReference::from_region(RegionId(3), 64).unwrap();
    assert_eq!(reference.region(), Ok(RegionId(3)));
    assert_eq!(reference.region_offset(), 64);
    assert_eq!(ObjectReference::from_region(RegionId(0), 7),
               Err(GcAddressError::Misaligned));
  }
}
