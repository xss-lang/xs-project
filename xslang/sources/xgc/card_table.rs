/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use super::REGION_SIZE_BYTES;

pub const CARD_SIZE_BYTES: usize = 512;
pub const CARD_COUNT: usize = REGION_SIZE_BYTES / CARD_SIZE_BYTES;

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct CardIndex(pub u16);

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum CardTableError
{
  OutsideRegion,
  CardIndexOverflow,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct CardTable
{
  dirty: Vec<bool>,
  dirty_count: usize,
}

impl Default for CardTable
{
  fn default() -> Self
  {
    Self::new()
  }
}

impl CardTable
{
  #[must_use]
  pub fn new() -> Self
  {
    Self { dirty: vec![false; CARD_COUNT],
           dirty_count: 0 }
  }

  pub fn card_for_offset(region_offset: usize) -> Result<CardIndex, CardTableError>
  {
    if region_offset >= REGION_SIZE_BYTES
    {
      return Err(CardTableError::OutsideRegion);
    }
    u16::try_from(region_offset / CARD_SIZE_BYTES).map(CardIndex)
                                                  .map_err(|_| CardTableError::CardIndexOverflow)
  }

  pub fn mark_offset(&mut self, region_offset: usize) -> Result<CardIndex, CardTableError>
  {
    let card = Self::card_for_offset(region_offset)?;
    self.mark(card)?;
    Ok(card)
  }

  pub fn mark(&mut self, card: CardIndex) -> Result<bool, CardTableError>
  {
    let index = usize::from(card.0);
    let Some(dirty) = self.dirty.get_mut(index)
    else
    {
      return Err(CardTableError::OutsideRegion);
    };
    let changed = !*dirty;
    *dirty = true;
    if changed
    {
      self.dirty_count += 1;
    }
    Ok(changed)
  }

  #[must_use]
  pub fn dirty_cards(&self) -> Vec<CardIndex>
  {
    self.dirty
        .iter()
        .enumerate()
        .filter_map(|(index, dirty)| dirty.then_some(index))
        .filter_map(|index| u16::try_from(index).ok())
        .map(CardIndex)
        .collect()
  }

  pub fn clear(&mut self)
  {
    self.dirty.fill(false);
    self.dirty_count = 0;
  }

  #[must_use]
  pub const fn dirty_count(&self) -> usize
  {
    self.dirty_count
  }
}

#[cfg(test)]
mod tests
{
  use super::*;

  #[test]
  fn card_table_deduplicates_dirty_cards()
  {
    let mut cards = CardTable::new();
    assert_eq!(cards.mark_offset(513), Ok(CardIndex(1)));
    assert_eq!(cards.mark_offset(900), Ok(CardIndex(1)));
    assert_eq!(cards.dirty_count(), 1);
    assert_eq!(cards.dirty_cards(), vec![CardIndex(1)]);
    cards.clear();
    assert_eq!(cards.dirty_count(), 0);
  }
}
