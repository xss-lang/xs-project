// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn choose(value: Long) -> Long {
    return value + 1;
}

fn choose(value: Int) -> Long {
    return 7;
}

fn main() -> Long {
    value: Long = 2;
    return choose(value) + choose(5);
}
