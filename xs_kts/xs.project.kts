/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

project("Example", "BETA", "0.1.0")

set("XS_VERSION", "0.1.6")
set("XS_BACKEND", "LLVM")

if (cfg(OS == LINUX && ARCH == X86_64)) {
  set("NATIVE_TARGET", "x86_64-unknown-linux-gnu")
}

authors(arrayOf("Leitwolf", "leitwolf@example.me"))

dependencies {
  addModule("https://github.com/xss-lang/externalModules/::JSON::0.1.0")
}

targets {
  target("x86_64-unknown-linux-gnu")
  target("aarch64-unknown-linux-gnu")
}

sources {
  include("sources/example/main.xs")
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
