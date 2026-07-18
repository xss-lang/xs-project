// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

use xslang::xlil::*;

#[test]
fn public_rust_api_builds_verifies_and_roundtrips_a_module()
{
  let mut function = Function::definition("main", Type::I32, Vec::new());
  let entry = function.append_block("entry");
  let result = function.add_const_i32(entry, 0)
                       .expect("public API should append const.i32");
  assert!(function.set_return(entry, Some(result)));

  let mut module = Module::new("PublicRustProducer");
  module.add_function(function);
  assert!(verify_module(&module).is_empty());

  let text = module_to_string(&module);
  let parsed = parse_module(&text).expect("public writer output should parse");
  assert!(verify_module(&parsed).is_empty());
  assert_eq!(parsed, module);
}
