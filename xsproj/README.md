<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xsproj

Public C23 parser/lexer/model project for `.xsproj` manifests.

This project is separate from the X# source parser. It exposes the public
`#include <xs/project.h>` API so external tools can read XS project manifests
without linking the whole compiler.

Implementation files live under `xsproj/sources/`.

`externalModules` records are manifest metadata only at this stage. Each
`addModule` block carries `moduleName`, `moduleRepo`, and `moduleVersion`; the
parser stores those fields but does not download or resolve external module
repositories yet. Public accessors expose the external module list and each
module field for third-party manifest readers.
