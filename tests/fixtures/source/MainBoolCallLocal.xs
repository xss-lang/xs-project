// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

fn IsZero(value: Long) -> Bool {
    return value == 0;
}

fn main() -> Long {
    flag: Bool = IsZero(1);
    return if (flag) { 3 } else { 7 };
}
