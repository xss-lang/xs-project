/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use xslang::xgc::*;

#[derive(Default)]
struct RuntimeBinding
{
  rewritten_roots: Vec<(RootLocation, Option<ObjectReference>)>,
}

impl XgcRuntimeBinding for RuntimeBinding
{
  fn object_layout(&self, metadata: TypeMetadataId) -> Option<ObjectLayout>
  {
    (metadata == TypeMetadataId(7)).then(|| ObjectLayout::new(24, 8).unwrap())
  }

  fn trace_references(&self, _: TypeMetadataId, _: ObjectReference, _: &mut dyn FnMut(ObjectReference)) {}

  fn rewrite_root(&mut self, location: RootLocation, reference: Option<ObjectReference>)
  {
    self.rewritten_roots.push((location, reference));
  }
}

#[test]
fn public_xgc_api_requires_an_explicit_runtime_binding()
{
  let binding = RuntimeBinding::default();
  let mut xgc = BoundXgc::new(XgcConfiguration::enabled(), binding);
  let layout = xgc.binding().object_layout(TypeMetadataId(7)).unwrap();
  assert_eq!(layout.size_bytes(), 24);

  let root = RootLocation { kind: RootKind::StackSlot,
                            index: 0 };
  xgc.binding_mut().rewrite_root(root, None);
  assert_eq!(xgc.into_binding().rewritten_roots, vec![(root, None)]);
}
