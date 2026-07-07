/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use serde::{Deserialize, Serialize};

#[derive(Debug, Deserialize, Serialize)]
struct XsAnalyzerConfig {
    trace: bool,
}

fn main() {
    let _config = XsAnalyzerConfig { trace: false };
    eprintln!("xs-analyzer language server is registered, but implementation has not started yet.");
}
