/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

import type { ExtensionContext } from "vscode";

export function activate(_context: ExtensionContext): void {
  // TextMate grammar registration is handled by package.json contributions.
}

export function deactivate(): void {
  // The Rust language server process is not started yet.
}
