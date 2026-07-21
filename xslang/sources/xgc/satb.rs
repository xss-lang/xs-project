/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

use super::ObjectReference;

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum SatbBufferError
{
  ZeroCapacity,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub enum SatbRecord
{
  Buffered,
  Publish(Vec<ObjectReference>),
}

/// Thread-local snapshot-at-the-beginning buffer.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct SatbBuffer
{
  capacity: usize,
  entries: Vec<ObjectReference>,
}

impl SatbBuffer
{
  pub fn new(capacity: usize) -> Result<Self, SatbBufferError>
  {
    if capacity == 0
    {
      return Err(SatbBufferError::ZeroCapacity);
    }
    Ok(Self { capacity,
              entries: Vec::with_capacity(capacity) })
  }

  pub fn record_old_reference(&mut self, reference: ObjectReference) -> SatbRecord
  {
    self.entries.push(reference);
    if self.entries.len() < self.capacity
    {
      return SatbRecord::Buffered;
    }
    SatbRecord::Publish(self.take())
  }

  #[must_use]
  pub fn take(&mut self) -> Vec<ObjectReference>
  {
    let mut replacement = Vec::with_capacity(self.capacity);
    std::mem::swap(&mut self.entries, &mut replacement);
    replacement
  }

  #[must_use]
  pub fn len(&self) -> usize
  {
    self.entries.len()
  }

  #[must_use]
  pub fn is_empty(&self) -> bool
  {
    self.entries.is_empty()
  }
}

#[cfg(test)]
mod tests
{
  use super::*;
  use crate::xgc::RegionId;

  #[test]
  fn full_buffer_publishes_old_references()
  {
    let first = ObjectReference::from_region(RegionId(0), 8).unwrap();
    let second = ObjectReference::from_region(RegionId(0), 16).unwrap();
    let mut buffer = SatbBuffer::new(2).unwrap();
    assert_eq!(buffer.record_old_reference(first), SatbRecord::Buffered);
    assert_eq!(buffer.record_old_reference(second),
               SatbRecord::Publish(vec![first, second]));
    assert!(buffer.is_empty());
  }
}
