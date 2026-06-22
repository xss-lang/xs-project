# XS project rules

- `.xsproj` files are XS project manifests.
- Do not replace the syntax with JSON, YAML, TOML or XML.
- Read the files under `./XS/Syntax/` before modifying XS source code.
- Do not invent unsupported language features.
- The project is built with:
  `xs build -proj MyApp.xsproj`
- The project is checked with:
  `xs check -proj MyApp.xsproj`
- The project is run with:
  `xs run -proj MyApp.xsproj`
- `compilerOptions.addFiles.entry` is the program entry source file.
- Other files inside `addFiles` are additional source files.
- Valid `appRelease` values are `ALPHA`, `BETA` and `STABLE`.
