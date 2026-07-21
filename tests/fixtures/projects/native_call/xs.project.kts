// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

project("NativeCall", "BETA", "0.2.1")
set("XGC_ENABLED", true)

dependencies {
  addModule("JSON", "stable", "0.1.0")
}

source {
  include("sources")
}
