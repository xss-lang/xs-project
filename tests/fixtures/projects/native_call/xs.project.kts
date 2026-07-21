// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

project("NativeCall", "BETA", "0.2.0")
set("XGC_ENABLED", true)

dependencies {
  addModule("JSON", "stable", "0.1.0")
}

source {
  include("sources")
}
