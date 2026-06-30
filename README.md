# xs-project

LLVM-project-style monorepo root for X#.

Buildable and planned projects live in sibling directories:

- `xs/`: X# compiler project.
- `xsproj/`: public C23 `.xsproj` manifest parser/lexer/model project.
- `xsfmt/`: future Rust nightly formatter project.
- `xstidy/`: future Rust nightly linter project.
- `xs-analyzer/`: future TypeScript VS Code extension project.
- `xs-backend/`: future native XS Backend project.

The root CMake project is named `xs_project` and is responsible for project
selection, toolchain policy and shared monorepo orchestration.
