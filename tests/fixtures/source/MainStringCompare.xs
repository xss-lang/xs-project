// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn same(left: Str, right: Str) -> Bool {
    return left == right;
}

fn different(left: Str, right: Str) -> Bool {
    return left != right;
}

fn main() -> Long {
    if (same("Leitwolf", "Leitwolf") && different("Leitwolf", "Alpha")) {
        return 0;
    } else {
        return 1;
    }
}
