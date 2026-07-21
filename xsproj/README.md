<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: MPL-2.0
-->

# xsproj

Public C23 parser/lexer/model project for `.xsproj` manifests.

This project is separate from the X# source parser. It exposes the public
`#include <xs/project.h>` API so external tools can read XS project manifests
without linking the whole compiler.

Implementation files live under `xsproj/sources/`.

The format and public API are feature-frozen. Existing application metadata,
source selection, compiler version, and output target records remain supported.
Dependency declarations belong to the programmable `xs.project.kts` system and
are not part of `.xsproj`.

`xs-proj Example.xsproj` parses and validates a manifest through this API. It is not a build command. Legacy manifest
compilation remains `xs build -proj Example.xsproj`; `-proj` is never used for Kotlin project files.
