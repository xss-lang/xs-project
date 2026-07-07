/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

import type { ExtensionContext } from "vscode";

export function activate(_context: ExtensionContext): void {
  console.info("XS Analyzer VS Code extension is registered, but implementation has not started yet.");
}

export function deactivate(): void {
  // The Rust language server process is not started yet.
}
