/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

use std::collections::BTreeSet;

use super::{CardIndex, RegionId};

const SMALL_LIMIT: usize = 8;
const MEDIUM_LIMIT: usize = 64;

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct RememberedCard
{
  pub region: RegionId,
  pub card: CardIndex,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum RememberedSetKind
{
  Small,
  Medium,
  Large,
}

#[derive(Clone, Debug, Eq, PartialEq)]
enum Storage
{
  Small(Vec<RememberedCard>),
  Medium(Vec<RememberedCard>),
  Large(BTreeSet<RememberedCard>),
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct RememberedSet
{
  storage: Storage,
}

impl Default for RememberedSet
{
  fn default() -> Self
  {
    Self::new()
  }
}

impl RememberedSet
{
  #[must_use]
  pub const fn new() -> Self
  {
    Self { storage: Storage::Small(Vec::new()) }
  }

  pub fn insert(&mut self, entry: RememberedCard) -> bool
  {
    match &mut self.storage
    {
      Storage::Small(entries) =>
      {
        if entries.contains(&entry)
        {
          return false;
        }
        entries.push(entry);
        if entries.len() > SMALL_LIMIT
        {
          entries.sort_unstable();
          self.storage = Storage::Medium(std::mem::take(entries));
        }
        true
      }
      Storage::Medium(entries) => match entries.binary_search(&entry)
      {
        Ok(_) => false,
        Err(index) =>
        {
          entries.insert(index, entry);
          if entries.len() > MEDIUM_LIMIT
          {
            self.storage = Storage::Large(std::mem::take(entries).into_iter().collect());
          }
          true
        }
      },
      Storage::Large(entries) => entries.insert(entry),
    }
  }

  pub fn remove(&mut self, entry: RememberedCard) -> bool
  {
    let removed = match &mut self.storage
    {
      Storage::Small(entries) => remove_vec_entry(entries, entry),
      Storage::Medium(entries) => match entries.binary_search(&entry)
      {
        Ok(index) =>
        {
          entries.remove(index);
          true
        }
        Err(_) => false,
      },
      Storage::Large(entries) => entries.remove(&entry),
    };
    self.shrink_representation();
    removed
  }

  fn shrink_representation(&mut self)
  {
    match &mut self.storage
    {
      Storage::Large(entries) if entries.len() <= MEDIUM_LIMIT =>
      {
        self.storage = Storage::Medium(std::mem::take(entries).into_iter().collect());
      }
      Storage::Medium(entries) if entries.len() <= SMALL_LIMIT =>
      {
        self.storage = Storage::Small(std::mem::take(entries));
      }
      _ =>
      {}
    }
  }

  #[must_use]
  pub fn contains(&self, entry: RememberedCard) -> bool
  {
    match &self.storage
    {
      Storage::Small(entries) => entries.contains(&entry),
      Storage::Medium(entries) => entries.binary_search(&entry).is_ok(),
      Storage::Large(entries) => entries.contains(&entry),
    }
  }

  #[must_use]
  pub fn entries(&self) -> Vec<RememberedCard>
  {
    let mut result = match &self.storage
    {
      Storage::Small(entries) | Storage::Medium(entries) => entries.clone(),
      Storage::Large(entries) => entries.iter().copied().collect(),
    };
    result.sort_unstable();
    result
  }

  #[must_use]
  pub fn len(&self) -> usize
  {
    match &self.storage
    {
      Storage::Small(entries) | Storage::Medium(entries) => entries.len(),
      Storage::Large(entries) => entries.len(),
    }
  }

  #[must_use]
  pub fn is_empty(&self) -> bool
  {
    self.len() == 0
  }

  #[must_use]
  pub const fn kind(&self) -> RememberedSetKind
  {
    match &self.storage
    {
      Storage::Small(_) => RememberedSetKind::Small,
      Storage::Medium(_) => RememberedSetKind::Medium,
      Storage::Large(_) => RememberedSetKind::Large,
    }
  }

  pub fn clear(&mut self)
  {
    self.storage = Storage::Small(Vec::new());
  }
}

fn remove_vec_entry(entries: &mut Vec<RememberedCard>, entry: RememberedCard) -> bool
{
  let Some(index) = entries.iter().position(|candidate| *candidate == entry)
  else
  {
    return false;
  };
  entries.remove(index);
  true
}

#[cfg(test)]
mod tests
{
  use super::*;

  fn entry(index: u16) -> RememberedCard
  {
    RememberedCard { region: RegionId(u32::from(index / 16)),
                     card: CardIndex(index) }
  }

  #[test]
  fn remembered_set_adapts_and_deduplicates()
  {
    let mut set = RememberedSet::new();
    for index in 0..=SMALL_LIMIT as u16
    {
      assert!(set.insert(entry(index)));
    }
    assert_eq!(set.kind(), RememberedSetKind::Medium);
    assert!(!set.insert(entry(0)));
    for index in (SMALL_LIMIT as u16 + 1)..=MEDIUM_LIMIT as u16
    {
      set.insert(entry(index));
    }
    assert_eq!(set.kind(), RememberedSetKind::Large);
    assert_eq!(set.len(), MEDIUM_LIMIT + 1);
  }
}
