// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: MPL-2.0

data Left {
    value: Long;
}

data Right {
    value: Long;
}

data Combined : Left, Right {
}

fn main() -> Long {
    combined: Combined = Combined {
        value: 1,
    };
    return combined.value;
}
