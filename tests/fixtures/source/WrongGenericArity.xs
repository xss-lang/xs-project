// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

fn identity<T>(value: T) -> T {
    return value;
}

fn main() -> Long {
    return identity::<Long, Int>(7);
}
