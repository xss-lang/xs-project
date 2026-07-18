// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

interface Marker {
}

interface DerivedMarker : Marker {
}

class Worker : DerivedMarker {
}

fn constrained<T: Marker>() -> Long {
    return 7;
}

fn main() -> Long {
    return constrained::<Worker>();
}
