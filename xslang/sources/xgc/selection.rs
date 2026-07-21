/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

use super::{GenerationId, REGION_SIZE_BYTES, RegionId, RegionMetadata, RegionState};

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CollectionPolicy
{
  pub maximum_regions: usize,
  pub minimum_garbage_ratio: f64,
  pub remembered_card_cost: f64,
}

impl Default for CollectionPolicy
{
  fn default() -> Self
  {
    Self { maximum_regions: 8,
           minimum_garbage_ratio: 0.20,
           remembered_card_cost: 64.0 }
  }
}

#[derive(Clone, Copy, Debug, PartialEq)]
pub struct CollectionCandidate
{
  pub region: RegionId,
  pub score: f64,
  pub reclaimable_bytes: usize,
  pub region_age: u64,
}

pub fn select_collection_set(regions: &[RegionMetadata],
                             remembered_card_counts: &[usize],
                             current_generation: GenerationId,
                             policy: CollectionPolicy)
                             -> Vec<CollectionCandidate>
{
  if policy.maximum_regions == 0 || regions.len() != remembered_card_counts.len()
  {
    return Vec::new();
  }
  let mut candidates: Vec<_> =
    regions.iter()
           .zip(remembered_card_counts)
           .filter_map(|(region, remembered)| candidate(region, *remembered, current_generation, policy))
           .collect();
  candidates.sort_by(|left, right| {
              right.score
                   .total_cmp(&left.score)
                   .then_with(|| left.region.cmp(&right.region))
            });
  candidates.truncate(policy.maximum_regions);
  candidates
}

fn candidate(region: &RegionMetadata,
             remembered_cards: usize,
             current_generation: GenerationId,
             policy: CollectionPolicy)
             -> Option<CollectionCandidate>
{
  if !matches!(region.state, RegionState::Survivor | RegionState::Old)
  {
    return None;
  }
  let reclaimable = region.garbage_bytes;
  let garbage_ratio = reclaimable as f64 / REGION_SIZE_BYTES as f64;
  if garbage_ratio < policy.minimum_garbage_ratio
  {
    return None;
  }
  let age = current_generation.0.saturating_sub(region.generation.0);
  let evacuation_cost = region.live_bytes.max(1) as f64;
  let remembered_cost = remembered_cards as f64 * policy.remembered_card_cost;
  let age_bonus = 1.0 + (age.min(100) as f64 / 100.0);
  let score = reclaimable as f64 * age_bonus / (evacuation_cost + remembered_cost);
  Some(CollectionCandidate { region: region.id,
                             score,
                             reclaimable_bytes: reclaimable,
                             region_age: age })
}

#[cfg(test)]
mod tests
{
  use super::*;

  fn old_region(id: u32, live: usize, garbage: usize, generation: u64) -> RegionMetadata
  {
    let mut region = RegionMetadata::free(RegionId(id));
    region.transition(RegionState::Eden, GenerationId(generation)).unwrap();
    region.transition(RegionState::Old, GenerationId(generation)).unwrap();
    region.update_usage(live, garbage, live + garbage).unwrap();
    region
  }

  #[test]
  fn selection_prefers_reclaimable_low_cost_regions()
  {
    let regions = vec![old_region(0, 100, 900, 1), old_region(1, 800, 900, 9)];
    let selected = select_collection_set(&regions,
                                         &[0, 20],
                                         GenerationId(10),
                                         CollectionPolicy { maximum_regions: 1,
                                                            minimum_garbage_ratio: 0.0,
                                                            ..CollectionPolicy::default() });
    assert_eq!(selected.len(), 1);
    assert_eq!(selected[0].region, RegionId(0));
  }
}
