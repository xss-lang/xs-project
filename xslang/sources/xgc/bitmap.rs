/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

use super::REGION_SIZE_BYTES;
use super::address::OBJECT_ALIGNMENT_BYTES;

const MARK_COUNT: usize = REGION_SIZE_BYTES / OBJECT_ALIGNMENT_BYTES as usize;
const WORD_BITS: usize = u64::BITS as usize;
const WORD_COUNT: usize = MARK_COUNT.div_ceil(WORD_BITS);

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum MarkBitmapError
{
  Misaligned,
  OutsideRegion,
}

/// One mark bit per aligned object-start position in a region.
#[derive(Clone, Debug, Eq, PartialEq)]
pub struct MarkBitmap
{
  words: Vec<u64>,
  marked: usize,
}

impl Default for MarkBitmap
{
  fn default() -> Self
  {
    Self::new()
  }
}

impl MarkBitmap
{
  #[must_use]
  pub fn new() -> Self
  {
    Self { words: vec![0; WORD_COUNT],
           marked: 0 }
  }

  fn bit(region_offset: usize) -> Result<(usize, u64), MarkBitmapError>
  {
    if region_offset >= REGION_SIZE_BYTES
    {
      return Err(MarkBitmapError::OutsideRegion);
    }
    if !region_offset.is_multiple_of(OBJECT_ALIGNMENT_BYTES as usize)
    {
      return Err(MarkBitmapError::Misaligned);
    }
    let index = region_offset / OBJECT_ALIGNMENT_BYTES as usize;
    Ok((index / WORD_BITS, 1_u64 << (index % WORD_BITS)))
  }

  pub fn mark(&mut self, region_offset: usize) -> Result<bool, MarkBitmapError>
  {
    let (word, mask) = Self::bit(region_offset)?;
    let changed = self.words[word] & mask == 0;
    self.words[word] |= mask;
    if changed
    {
      self.marked += 1;
    }
    Ok(changed)
  }

  pub fn unmark(&mut self, region_offset: usize) -> Result<bool, MarkBitmapError>
  {
    let (word, mask) = Self::bit(region_offset)?;
    let changed = self.words[word] & mask != 0;
    self.words[word] &= !mask;
    if changed
    {
      self.marked -= 1;
    }
    Ok(changed)
  }

  pub fn is_marked(&self, region_offset: usize) -> Result<bool, MarkBitmapError>
  {
    let (word, mask) = Self::bit(region_offset)?;
    Ok(self.words[word] & mask != 0)
  }

  pub fn clear(&mut self)
  {
    self.words.fill(0);
    self.marked = 0;
  }

  #[must_use]
  pub const fn marked_count(&self) -> usize
  {
    self.marked
  }
}

#[cfg(test)]
mod tests
{
  use super::*;

  #[test]
  fn bitmap_tracks_object_starts()
  {
    let mut bitmap = MarkBitmap::new();
    assert_eq!(bitmap.mark(16), Ok(true));
    assert_eq!(bitmap.mark(16), Ok(false));
    assert_eq!(bitmap.is_marked(16), Ok(true));
    assert_eq!(bitmap.marked_count(), 1);
    assert_eq!(bitmap.unmark(16), Ok(true));
    assert_eq!(bitmap.mark(3), Err(MarkBitmapError::Misaligned));
  }
}
