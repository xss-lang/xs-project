// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn IsPositive(value: Long) -> Bool {
    return value > 0;
}

fn main() -> Long {
    return if (IsPositive(7)) { 7; } else { 3; };
}
