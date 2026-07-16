/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use std::collections::BTreeMap;

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct SafepointId(pub u32);

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub enum RootKind
{
  StackSlot,
  Register,
  Global,
  ThreadLocal,
}

#[derive(Clone, Copy, Debug, Eq, Hash, Ord, PartialEq, PartialOrd)]
pub struct RootLocation
{
  pub kind: RootKind,
  pub index: u32,
}

#[derive(Clone, Debug, Eq, PartialEq)]
pub struct StackMap
{
  pub function: String,
  pub safepoint: SafepointId,
  roots: Vec<RootLocation>,
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum StackMapError
{
  EmptyFunction,
  DuplicateSafepoint,
}

impl StackMap
{
  pub fn new(function: impl Into<String>,
             safepoint: SafepointId,
             mut roots: Vec<RootLocation>)
             -> Result<Self, StackMapError>
  {
    let function = function.into();
    if function.is_empty()
    {
      return Err(StackMapError::EmptyFunction);
    }
    roots.sort_unstable();
    roots.dedup();
    Ok(Self { function,
              safepoint,
              roots })
  }

  #[must_use]
  pub fn roots(&self) -> &[RootLocation]
  {
    &self.roots
  }
}

#[derive(Clone, Debug, Default, Eq, PartialEq)]
pub struct StackMapRegistry
{
  maps: BTreeMap<(String, SafepointId), StackMap>,
}

impl StackMapRegistry
{
  pub fn insert(&mut self, map: StackMap) -> Result<(), StackMapError>
  {
    let key = (map.function.clone(), map.safepoint);
    if self.maps.contains_key(&key)
    {
      return Err(StackMapError::DuplicateSafepoint);
    }
    self.maps.insert(key, map);
    Ok(())
  }

  #[must_use]
  pub fn find(&self, function: &str, safepoint: SafepointId) -> Option<&StackMap>
  {
    self.maps.get(&(function.to_owned(), safepoint))
  }

  #[must_use]
  pub fn len(&self) -> usize
  {
    self.maps.len()
  }

  #[must_use]
  pub fn is_empty(&self) -> bool
  {
    self.maps.is_empty()
  }
}

#[cfg(test)]
mod tests
{
  use super::*;

  #[test]
  fn registry_keeps_precise_deduplicated_roots()
  {
    let root = RootLocation { kind: RootKind::StackSlot,
                              index: 2 };
    let map = StackMap::new("main", SafepointId(1), vec![root, root]).unwrap();
    assert_eq!(map.roots(), &[root]);
    let mut registry = StackMapRegistry::default();
    assert_eq!(registry.insert(map.clone()), Ok(()));
    assert_eq!(registry.insert(map), Err(StackMapError::DuplicateSafepoint));
  }
}
