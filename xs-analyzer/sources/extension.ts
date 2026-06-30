import type { ExtensionContext } from "vscode";

export function activate(_context: ExtensionContext): void {
  console.info("XS Analyzer extension is registered, but implementation has not started yet.");
}

export function deactivate(): void {
  // No analyzer resources exist yet.
}
