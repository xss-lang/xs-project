/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

mod address;
mod bitmap;
mod card_table;
mod heap;
mod model;
mod remembered_set;
mod roots;
mod satb;
mod selection;
mod telemetry;

pub use address::{GcAddressError, HeapOffset, ObjectReference};
pub use bitmap::{MarkBitmap, MarkBitmapError};
pub use card_table::{CARD_COUNT, CARD_SIZE_BYTES, CardIndex, CardTable, CardTableError};
pub use heap::{HumongousAllocation, RegionDirectory, RegionDirectoryError, RegionRecord};
pub use model::{
  GenerationId, HUMONGOUS_OBJECT_THRESHOLD_BYTES, REGION_SIZE_BYTES, RegionId, RegionMetadata, RegionModelError,
  RegionState, XgcConfiguration,
};
pub use remembered_set::{RememberedCard, RememberedSet, RememberedSetKind};
pub use roots::{RootKind, RootLocation, SafepointId, StackMap, StackMapError, StackMapRegistry};
pub use satb::{SatbBuffer, SatbBufferError, SatbRecord};
pub use selection::{CollectionCandidate, CollectionPolicy, select_collection_set};
pub use telemetry::{Telemetry, TelemetrySnapshot};
