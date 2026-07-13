/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

project("Example", "BETA", "0.1.0")

set("XS_VERSION", "0.1.6")
set("XS_BACKEND", "LLVM")

if (cfg(OS == LINUX)) {
  set("PLATFORM", "linux")
} else if (cfg(OS == MACOS)) {
  set("PLATFORM", "macos")
} else if (cfg(FAMILY == BSD)) {
  set("PLATFORM", "bsd")
} else if (cfg(OS == WINDOWS)) {
  set("PLATFORM", "windows")
} else {
  panic("unsupported operating system")
}

if (cfg(FAMILY == UNIX)) {
  set("UNIX_FAMILY", "true")
}

if (cfg(ARCH == X86_64)) {
  set("NATIVE_ARCH", "x86_64")
} else if (cfg(ARCH == AARCH64)) {
  set("NATIVE_ARCH", "aarch64")
} else if (cfg(ARCH == ARMV7H)) {
  set("NATIVE_ARCH", "armv7h")
} else if (cfg(ARCH == RISCV64)) {
  set("NATIVE_ARCH", "riscv64")
} else {
  panic("unsupported architecture")
}

authors(
  arrayOf("Leitwolf", "leitwolf@example.me"),
  arrayOf("Helmut", "helmut@example.me"),
)

dependencies {
  addModule("https://github.com/xss-lang/externalModules/::JSON::0.1.0")
  addModule("https://github.com/xss-lang/externalModules/::XML::0.1.0")
}

targets {
  target("x86_64-unknown-linux-gnu")
  target("x86_64-unknown-linux-musl")
  target("aarch64-unknown-linux-gnu")
  target("riscv64gc-unknown-linux-gnu")
  target("x86_64-apple-darwin")
  target("aarch64-apple-darwin")
  target("x86_64-pc-windows-msvc")
  target("aarch64-pc-windows-msvc")
  target("x86_64-unknown-freebsd")
  target("aarch64-unknown-freebsd")
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
