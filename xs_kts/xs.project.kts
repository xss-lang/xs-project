/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

project("Example", "BETA", "0.1.0")

set("XS_VERSION", "0.1.7")
set("XS_BACKEND", "LLVM")

set(
  "TARGET",
  "x86_64-unknown-linux-gnu",
  "x86_64-unknown-linux-musl",
  "aarch64-unknown-linux-gnu",
  "armv7h-unknown-linux-gnueabihf",
  "riscv64gc-unknown-linux-gnu",
  "x86_64-apple-darwin",
  "aarch64-apple-darwin",
  "x86_64-pc-windows-msvc",
  "aarch64-pc-windows-msvc",
  "x86_64-unknown-freebsd",
  "aarch64-unknown-freebsd",
)

println("Target: ${get("TARGET")}")
println("Project: ${get("PROJECT")}")
println("X# version: ${get("XS_VERSION")}")
println("Backend: ${get("XS_BACKEND")}")

authors(
  arrayOf("Leitwolf", "leitwolf@example.me"),
  arrayOf("Helmut", "helmut@example.me"),
)

dependencies {
  addModule("https://github.com/xss-lang/externalModules/::JSON::0.1.0")
  addModule("https://github.com/xss-lang/externalModules/::XML::0.1.0")
}

sources {
  include("sources/**/*.xs")
  exclude("sources/tests/**")
}

test {
  include("tests/**/*.xs")
  framework("xs-test")
}

compiler {
  warnings("all")
  werror(true)
  verbose(false)
}
