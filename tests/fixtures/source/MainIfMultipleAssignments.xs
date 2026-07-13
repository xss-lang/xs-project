// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn main() -> Long {
    value: Long = 1;
    enabled: Bool = true;
    if (enabled) {
        value += 6;
        value ^= 0;
    } else {
        value += 2;
        value *= 2;
    }
    return value;
}
