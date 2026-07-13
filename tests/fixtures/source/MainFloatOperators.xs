// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

fn single_add(a: SFloat, b: SFloat) -> SFloat {
    return a + b;
}

fn single_remainder(a: SFloat, b: SFloat) -> SFloat {
    return a % b;
}

fn double_divide(a: Float, b: Float) -> Float {
    return a / b;
}

fn double_less(a: Float, b: Float) -> Bool {
    return a < b;
}

fn main() -> Long {
    if (double_less(1.5, 2.5)) {
        return 0;
    } else {
        return 1;
    }
}
