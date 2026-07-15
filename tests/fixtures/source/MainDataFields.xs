// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

data Point {
    x: Long;
    y: Long;
}

fn main() -> Long {
    point: Point = Point {
        x: 2,
        y: 3,
    };
    point.x += 4;
    return point.x + point.y;
}
