// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

interface Marker {
}

class Other {
}

fn constrained<T: Marker>() -> Long {
    return 7;
}

fn main() -> Long {
    return constrained::<Other>();
}
