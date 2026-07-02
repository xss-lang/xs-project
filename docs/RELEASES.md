# Release policy

xs-project does not produce numbered releases yet. The release period will begin after LLVM IR generation is working.

The reason is the pipeline boundary: while parser, AST, macro, HIR, MIR, borrow checker, XLIL, and LLVM backend layers are
being developed incrementally, the project should not promise a half-complete native compiler under a versioned release name.

## Current status

- The root [../CHANGELOG.md](../CHANGELOG.md) file is an `Unreleased` development log.
- The `Unreleased` heading does not mean a stable release.
- Commit and test history show implementation progress.

## First release threshold

The expected minimum threshold for the first numbered release:

1. function body lowering from typed, borrow-checked MIR to XLIL,
2. XLIL to LLVM IR function body lowering,
3. LLVM module verification,
4. object emission,
5. linking according to project targets,
6. native artifact generation through `xs build`.

After this threshold is reached, user-visible changes accumulated under `Unreleased` may be moved into the first release
heading.
