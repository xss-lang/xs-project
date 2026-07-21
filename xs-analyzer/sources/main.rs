/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

mod server;

fn main() {
    if let Err(error) = server::run() {
        eprintln!("xs-analyzer: {error}");
        std::process::exit(1);
    }
}
