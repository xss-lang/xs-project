# xsproj

Public C23 parser/lexer/model project for `.xsproj` manifests.

This project is separate from the X# source parser. It exposes the public
`#include <xs/project.h>` API so external tools can read XS project manifests
without linking the whole compiler.

Implementation files live under `xsproj/sources/`.
