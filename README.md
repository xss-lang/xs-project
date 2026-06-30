# xs-project

LLVM-project-style monorepo root for X#.

Buildable and planned projects live in sibling directories:

- `include/`: shared public C headers used by multiple projects.
- `xs/`: X# compiler project.
- `xsproj/`: public C23 `.xsproj` manifest parser/lexer/model project.
- `xsfmt/`: future Rust nightly formatter project.
- `xstidy/`: future Rust nightly linter project.
- `xs-analyzer/`: future Rust language server and TypeScript VS Code extension project.
- `xs-backend/`: future native XS Backend project.

Developer tool user configuration, including `xsfmt`, `xstidy` and future tool
projects, is standardized on TOML. `.xsproj` remains the separate XSPROJ
manifest syntax.

The root CMake project is named `xs_project` and is responsible for project
selection, toolchain policy and shared monorepo orchestration.
