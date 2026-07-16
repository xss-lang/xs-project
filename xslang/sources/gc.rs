/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

mod model;

pub use model::{
  GenerationId, HUMONGOUS_OBJECT_THRESHOLD_BYTES, REGION_SIZE_BYTES, RegionId, RegionMetadata, RegionModelError,
  RegionState, XgcConfiguration,
};
