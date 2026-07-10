// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Module search and import resolution:

//
// This file defines:
//
// - Project-root detection
// - Recursive module-file search
// - Module declaration requirements
// - Module-name uniqueness
// - Import resolution
// - Build-graph interaction
// - HIR dependency-graph construction
//
// Module search is based on declared module names.
//
// Module search is not based on source file names.
//

// ============================================================
// Project root
// ============================================================

// The directory containing the active .xsproj file is the
// project root.


// Example:
//
// MyProject/
// ├── MyApp.xsproj
// ├── source/
// │   └── Main.xs
// └── modules/
//     └── MathUtils.xs


// Command:

// xs build -proj MyProject/MyApp.xsproj


// Project root:
//
// MyProject/


// Relative paths inside the project manifest are resolved from
// the project root.


// ============================================================
// Search scope
// ============================================================

// Module-file search starts at the project root.
//
// The search is recursive.
//
// Every directory below the project root may be searched.
//
// Only .xs source files participate in module discovery.


// Conceptual search:
//
// project root
//     ↓
// recursive directory traversal
//     ↓
// every .xs file
//     ↓
// inspect module declaration
//     ↓
// register declared module name


// Files outside the project root are not discovered by the
// project-root recursive search.


// ============================================================
// Module declarations
// ============================================================

// A module declaration is optional for a directly compiled source
// file.


// Valid direct source without module:

fn Main() {
}


// A file that is imported must declare a module.


// Importable source:

module Math;

fn Add(a: Int, b: Int) => Int {
    return a + b;
}


// When present, module must be the first declaration in the file.


// Valid:

module Math;

fn Add(a: Int, b: Int) => Int {
    return a + b;
}


// Invalid:

fn Add(a: Int, b: Int) => Int {
    return a + b;
}

module Math;


// A file without a module declaration cannot be resolved as an
// import target.


// ============================================================
// File name and module name
// ============================================================

// The source file name and declared module name may differ.


// File:
//
// modules/MathUtils.xs


// Contents:

module Math;


// Import:

imports Math;


// This is valid.


// Matching file and module names is recommended but not required.


// Recommended:
//
// File: Math.xs
//
// module Math;


// Import resolution uses:
//
// module Math;
//
// It does not use:
//
// MathUtils.xs


// ============================================================
// Module discovery
// ============================================================

// During module discovery, the compiler reads the beginning of each
// recursively discovered .xs file and checks for a module declaration.


// Conceptual discovered-module record:

data DiscoveredModule {
    moduleName: Str
    sourcePath: Str
}


// Example:

// File:
//
// modules/math/MathUtils.xs


// Declaration:

module Math;


// Registry entry:
//
// moduleName: "Math"
// sourcePath: "modules/math/MathUtils.xs"


// Files without module declarations are not added to the module
// registry.


// ============================================================
// Unique module names
// ============================================================

// A module name must be unique inside the project-root search scope.


// Valid:

// File: modules/Math.xs

module Math;


// File: modules/IO.xs

module IO;


// Invalid duplicate module name:

// File: first/Math.xs

module Math;


// File: second/OtherFile.xs

module Math;


// Duplicate module names produce a compile-time error.


// The file names being different does not remove the conflict.


// ============================================================
// Import declarations
// ============================================================

imports Math;


// imports resolves the declared module name.


// imports does not:
//
// - Resolve by source file name
// - Copy source text
// - Behave like #include
// - Automatically add a file to addFiles
// - Create a textual translation unit


// imports does:
//
// - Find the module in the discovered-module registry
// - Make the module available for name resolution
// - Add a dependency edge to the HIR dependency graph
// - Make accessible declarations available according to
//   visibility rules


// ============================================================
// Import resolution process
// ============================================================

// Conceptual process:
//
// 1. Determine the project root from the .xsproj file.
//
// 2. Recursively discover .xs files below the project root.
//
// 3. Read module declarations.
//
// 4. Build a module-name registry.
//
// 5. Reject duplicate module names.
//
// 6. For every imports declaration, look up the declared
//    module name.
//
// 7. If found, connect the importer and imported module in the
//    HIR dependency graph.
//
// 8. If not found, emit a compile-time diagnostic.


// ============================================================
// Successful import
// ============================================================

// Project:

// MyProject/
// ├── MyApp.xsproj
// ├── source/
// │   └── Main.xs
// └── libraries/
//     └── math/
//         └── MathUtils.xs


// libraries/math/MathUtils.xs:

module Math;

public fn Add(a: Int, b: Int) => Int {
    return a + b;
}


// source/Main.xs:

imports Math;

fn Main() {
    value: Int = Math.Add(2, 3);
}


// Resolution:
//
// imports Math
//     ↓
// recursive module registry lookup
//     ↓
// module Math
//     ↓
// libraries/math/MathUtils.xs


// ============================================================
// Missing module
// ============================================================

// Invalid when no recursively discovered .xs file declares Math:

imports Math;


// Result:
//
// - Compile-time error
// - Diagnostic points to the imports declaration
// - No file-name guessing is performed


// A file named Math.xs is not enough by itself.
//
// It must declare:

module Math;


// ============================================================
// Direct build sources
// ============================================================

// Files listed in .xsproj addFiles are direct build-graph sources.


// Example:

addFiles {
    entry: "source/Main.xs"

    [
        "source/Foo.xs",
        "source/Bar.xs"
    ]
}


// Direct build source:
//
// - Is explicitly part of the project build graph
// - Does not require a module declaration
// - May still declare a module when desired


// ============================================================
// Imported sources
// ============================================================

// A source file does not need to appear in addFiles to be imported.


// Example:
//
// modules/Math.xs is not listed in addFiles.
//
// It declares:
//
// module Math;


// A direct source may use:

imports Math;


// The compiler finds it through recursive project-root module
// discovery.


// Therefore:
//
// addFiles
//     Explicit direct build-graph inputs.
//
// imports
//     Module dependency resolution through declared module names.


// These mechanisms are related but are not identical.


// ============================================================
// No textual inclusion
// ============================================================

// X# module import is not C/C++ header inclusion.


// X# .xs source file:
//
// - Is a complete source file
// - May contain definitions and function bodies
// - Is not a header file
// - Is not copied into the importing source
// - Does not require include guards
// - Does not require pragma once


// imports performs semantic module resolution.


// ============================================================
// HIR dependency graph
// ============================================================

// Import resolution belongs to HIR.
//
// The parser only creates ImportDeclaration AST nodes.


// Parser result:

// ImportDeclaration
// └── modulePath: Math


// HIR resolves:

// ImportDeclaration(Math)
//     ↓
// ModuleSymbol(Math)
//     ↓
// SourceFile(modules/Math.xs)


// HIR dependency graph tracks:
//
// - Module dependencies
// - Import dependencies
// - Type dependencies
// - Function-call dependencies
// - Trait/interface dependencies
// - Generic-use dependencies


// Example dependency edge:

// Main module
//     ──imports──> Math module


// ============================================================
// Dependency invalidation
// ============================================================

// If a tracked module or declaration changes, dependent codegen
// units are invalidated through the dependency graph.


// This applies to:
//
// - Public declarations
// - Private declarations
// - Function signatures
// - Function bodies
// - Types
// - Generic declarations
// - Traits/interfaces
// - Module declarations
// - Import relationships


// ============================================================
// Search order
// ============================================================

// Module search is based on the complete recursively discovered
// registry.
//
// Directory traversal order does not choose between duplicates.
//
// If two files declare the same module name:
//
// - The compiler does not select the first one.
// - The compiler reports a duplicate-module error.


// ============================================================
// Search exclusions
// ============================================================

// Only .xs files are module candidates.


// The following are not module source candidates:
//
// - .xsproj
// - .txt
// - object files
// - libraries
// - generated executable files
// - unrelated file extensions


// ============================================================
// Case sensitivity
// ============================================================

// Module names follow normal identifier and path rules.
//
// Module-name matching is case-sensitive.


// These are different names:

module Math;

module math;


// Import must match the declared module name exactly:

imports Math;


// This does not resolve module Math:

imports math;


// ============================================================
// Nested module paths
// ============================================================

// Module paths are resolved from the complete declared module path.


// Example:

module Math.Advanced.Algebra;


// Import:

imports Math.Advanced.Algebra;


// The physical directory structure does not need to match the
// declared module path.


// Valid file:
//
// arbitrary/location/AlgebraModule.xs


// Contents:

module Math.Advanced.Algebra;


// Resolution still uses the declared module path.


// ============================================================
// Namespace distinction
// ============================================================

// module identifies an importable source module.
//
// namespace organizes declarations inside a module.


// Example:

module Math;
namespace Advanced;
namespace Algebra;


// Effective namespace path:
//
// Math.Advanced.Algebra


// imports resolves the module name:

imports Math;


// Namespace resolution is performed after module resolution in HIR.


// ============================================================
// Diagnostics
// ============================================================

// Module-search diagnostics include:
//
// - Imported module not found
// - Duplicate module declaration
// - Module declaration not first
// - Invalid module path
// - Import target without a module declaration
// - Inaccessible imported declaration
// - Import dependency cycle when such a cycle violates later
//   semantic rules


// Every diagnostic must retain SourceSpan information.


// ============================================================
// Parser responsibilities
// ============================================================

// The parser:
//
// - Parses module declarations
// - Parses import declarations
// - Verifies module is syntactically first when present
// - Produces structural AST nodes
// - Preserves SourceSpan


// The parser does not:
//
// - Search directories
// - Open import targets
// - Resolve module names
// - Validate module uniqueness
// - Build the semantic dependency graph


// ============================================================
// HIR responsibilities
// ============================================================

// HIR:
//
// - Builds the project module registry
// - Resolves imports
// - Rejects duplicate module names
// - Resolves namespaces
// - Performs name resolution
// - Builds dependency-graph edges
// - Applies visibility rules


// ============================================================
// Complete algorithm
// ============================================================

//
// input:
//     active .xsproj path
//
// root:
//     directory containing the .xsproj file
//
// discovery:
//     recursively traverse root
//     inspect every .xs file
//     collect declared module names
//
// validation:
//     reject duplicate module names
//
// resolution:
//     map imports paths to declared module names
//
// graph:
//     add import edges to the HIR dependency graph
//
// failure:
//     emit compile-time diagnostics
//
// ============================================================
// Summary
// ============================================================

//
// Project root:
//
//     Directory containing the active .xsproj file.
//
// Search:
//
//     Project root + recursive traversal.
//
// Candidate files:
//
//     .xs only.
//
// Import key:
//
//     Declared module name.
//
// File-name matching:
//
//     Not used.
//
// Module declaration:
//
//     Optional for direct sources.
//     Required for importable sources.
//     Must be first when present.
//
// Duplicate module names:
//
//     Compile-time error.
//
// addFiles:
//
//     Adds direct build-graph sources.
//
// imports:
//
//     Resolves modules semantically.
//     Does not perform textual inclusion.
//
// Resolution stage:
//
//     HIR.
//