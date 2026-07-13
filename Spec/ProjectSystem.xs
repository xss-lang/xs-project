// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Project system:

//
// The project system defines:
//
// - .xsproj project manifests
// - Project metadata
// - Compiler options
// - Entry selection
// - Build-graph source files
// - Output targets
// - External modules
// - Project CLI commands
// - Module declarations
// - Import resolution
// - Module and build-graph interaction
//


// ============================================================
// Project manifest
// ============================================================

// XS project manifests use the .xsproj extension.
//
// The file name is unrestricted.
//
// .xsproj files use their own lexer and parser.
// They are not parsed by the .xs source lexer/parser.
//
// Supported comments:
//
// //  line comment
// /// documentation-style line comment
//
// Multiline comments are not supported in .xsproj files.
//
// Examples:
//
// MyApp.xsproj
// Foo.xsproj
// Project.xsproj


// ============================================================
// Project commands
// ============================================================

// Check a project:

// xs check -proj Foo.xsproj


// Build a project:

// xs build -proj Foo.xsproj


// Build and run a project:

// xs run -proj Foo.xsproj


// -proj must be followed by the .xsproj file path.


// ============================================================
// Complete project example
// ============================================================

appName: "MyApp"
appVersion: "0.1.0"
appRelease: "BETA"
appLicense: "MIT"

appAuthors {
    ["Alpha", "alfa@example.me"]
    ["Foo", "foo@baz.me"]
}

compilerOptions {
    xsVersion: "26"
    xsBackend: "LLVM"

    addFiles {
        entry: "source/Main.xs"

        [
            "source/Foo.xs",
            "source/Bar.xs"
        ]
    }

    output {
        [
            osName: "Linux"
            osArch: "x64"
        ]
    }
}

externalModules {
    addModule {
        moduleName: "xsmods.JSON"
        moduleRepo: "https://github.com/xss-lang/externalModules"
        moduleVersion: "0.1"
    }
}


// ============================================================
// Separators
// ============================================================

// Fields may be separated by a newline:

compilerOptions {
    xsVersion: "26"
}


// Fields on the same line must be separated with semicolon:

compilerOptions {
    xsVersion: "26"
}


// A trailing semicolon is allowed:

compilerOptions {
    xsVersion: "26";
}


// Repeated semicolons are invalid:

compilerOptions {
    xsVersion: "26";;
}

// ============================================================
// Field rules
// ============================================================

// Field names are case-sensitive.
//
// Example:
//
// appName
//
// AppName and APPNAME are different names and are not valid
// replacements for appName.


// Unknown fields are compile-time errors:

appNames: "MyApp"


// Defining the same field more than once is a compile-time error:

appName: "MyApp"
appName: "OtherApp"


// ============================================================
// Required top-level fields
// ============================================================

// Required:
//
// - appName
// - appVersion
// - appRelease
// - appLicense
// - appAuthors
// - compilerOptions
//
// Optional:
//
// - externalModules


// ============================================================
// Project metadata
// ============================================================

appName: "MyApp"
appVersion: "0.1.0"
appRelease: "BETA"
appLicense: "MIT"


// Valid appRelease values:
//
// - "ALPHA"
// - "BETA"
// - "STABLE"


// Metadata values may be None:

appName: None
appVersion: None
appRelease: None
appLicense: None


// None means that the value does not exist.
//
// None metadata does not prevent output when compilerOptions,
// entry selection and output targets are valid.


// ============================================================
// Authors
// ============================================================

appAuthors {
    ["Alpha", "alfa@example.me"]
    ["Foo", None]
}


// appAuthors is required.
//
// appAuthors must contain at least one author record.
//
// Each author record must contain exactly two values:
//
// [
//     author name,
//     author email
// ]
//
// Either value may be None.


// Valid:

appAuthors {
    ["Alpha", None]
}


// Also valid:

appAuthors {
    [None, None]
}


// Invalid because the record contains only one value:

appAuthors {
    ["Alpha"]
}


// Invalid because no author record exists:

appAuthors {
}


// ============================================================
// Compiler options
// ============================================================

compilerOptions {
    xsVersion: "26"
    xsBackend: "LLVM"

    addFiles {
        entry: "source/Main.xs"
    }

    output {
        [
            osName: "Linux"
            osArch: "x64"
        ]
    }
}


// compilerOptions is required.
//
// Required compilerOptions fields:
//
// - xsVersion
// - addFiles
// - output


// Optional compilerOptions fields:
//
// - xsBackend
//
// xsBackend selects a future compiler backend preference.
//
// Valid xsBackend values:
//
// - "LLVM"
// - "XS"
//
// This setting is accepted for forward compatibility.
// It does not affect compilation behavior yet.


// ============================================================
// XS version
// ============================================================

compilerOptions {
    xsVersion: "26"

    addFiles {
        entry: "source/Main.xs"
    }

    output {
        [
            osName: "Linux"
            osArch: "x64"
        ]
    }
}


// xsVersion selects the X# language version.


// None version:

compilerOptions {
    xsVersion: None

    addFiles {
        entry: "source/Main.xs"
    }

    output {
        [
            osName: "Linux"
            osArch: "x64"
        ]
    }
}


// When xsVersion is None:
//
// - No error occurs.
// - No X# version is selected.
// - No compilation output is produced.


// ============================================================
// Source files and entry
// ============================================================

compilerOptions {
    xsVersion: "26"

    addFiles {
        entry: "source/Main.xs"

        [
            "source/Foo.xs",
            "source/Bar.xs"
        ]
    }

    output {
        [
            osName: "Linux"
            osArch: "x64"
        ]
    }
}


// addFiles is required.
//
// entry is required as a field, but its value may be None.
//
// The list below entry is optional.


// When entry contains a path:

addFiles {
    entry: "source/Main.xs"

    [
        "source/Foo.xs",
        "source/Bar.xs"
    ]
}


// Selection:
//
// entry:
//     source/Main.xs
//
// additional build-graph files:
//     source/Foo.xs
//     source/Bar.xs


// When entry is None and the list contains files:

addFiles {
    entry: None

    [
        "source/Foo.xs",
        "source/Bar.xs"
    ]
}


// Selection:
//
// entry:
//     source/Foo.xs
//
// additional build-graph files:
//     source/Bar.xs


// When entry is None and no file list exists:

addFiles {
    entry: None
}


// Result:
//
// - No error occurs.
// - No entry is selected.
// - No compilation output is produced.


// The additional file list may be omitted:

addFiles {
    entry: "source/Main.xs"
}


// ============================================================
// Build graph
// ============================================================

// Files listed by addFiles are added directly to the project
// build graph.
//
// The selected entry file is the program entry.
//
// Other listed files are additional build-graph source files.


// imports does not add a file directly to the project build graph.
//
// imports performs module and symbol resolution.
//
// imports is not a text-inclusion system.


// A .xs file outside addFiles:
//
// - Is not directly added as a project build input.
// - Is still a normal X# source file.
// - May contain complete definitions and function bodies.
// - May be used through imports when it declares a module.


// .xs files are not C or C++ header files.
//
// Differences:
//
// - .xs files are complete source files.
// - They are not limited to declarations.
// - imports does not copy source text.
// - There is no #include-style textual inclusion.
// - Include guards are not required.
// - pragma once is not required.


// ============================================================
// Output targets
// ============================================================

output {
    [
        osName: "Linux"
        osArch: "x64"
    ]
}


// output is required.
//
// output must contain at least one target.
//
// Every target requires:
//
// - osName
// - osArch


// Multiple output targets may be declared:

output {
    [
        osName: "Linux"
        osArch: "x64"
    ]

    [
        osName: "Windows"
        osArch: "x64"
    ]
}


// Values may be None:

output {
    [
        osName: None
        osArch: None
    ]
}


// None values do not cause a manifest syntax error.
//
// Output is only produced when the compiler has enough valid
// compiler and target information.


// ============================================================
// External modules
// ============================================================

externalModules {
    addModule {
        moduleName: "xsmods.JSON"
        moduleRepo: "https://github.com/xss-lang/externalModules"
        moduleVersion: "0.1"
    }

    addModule {
        moduleName: "xsmods.Xml"
        moduleRepo: "https://github.com/xss-lang/externalModules"
        moduleVersion: "1.2.0"
    }
}


// externalModules is optional.
//
// Only one externalModules block may exist.
//
// An externalModules block may be empty:

externalModules {
}


// externalModules may contain multiple addModule blocks.
//
// Each addModule requires:
//
// - moduleName
// - moduleRepo
// - moduleVersion


// Values may be None:

externalModules {
    addModule {
        moduleName: None
        moduleRepo: None
        moduleVersion: None
    }
}


// Fields belonging to addModule cannot be written directly inside
// externalModules.


// Invalid:

externalModules {
    moduleName: "xs.Xml"

    addModule {
        moduleName: "xs.Json"
        moduleRepo: "https://github.com/xss-lang/externalModules"
        moduleVersion: "latest"
    }
}


// Duplicate fields inside addModule are invalid:

externalModules {
    addModule {
        moduleName: "xs.Json"
        moduleName: "xs.Xml"
        moduleRepo: "https://github.com/xss-lang/externalModules"
        moduleVersion: "latest"
    }
}


// Multiple externalModules blocks are invalid:

externalModules {
}

externalModules {
}


// ============================================================
// None project behavior
// ============================================================

// Every value position in .xsproj may contain None.
//
// None means absence.
//
// A manifest in which every value is None:
//
// - Is not a syntax error.
// - Does not produce compilation output.


// Metadata may remain None while output is still produced,
// provided the required compiler configuration, entry selection
// and output target information are usable.


// ============================================================
// Module declarations
// ============================================================

// A module declaration is optional in general.


// A directly added build-graph file does not require a module:

fn Main() {
}


// A file that must be imported requires a module declaration:

module Math;

fn Add(a: Int, b: Int) -> Int {
    return a + b;
}


// When present, module must be the first declaration in the file.


// Valid:

module Math;

fn Add(a: Int, b: Int) -> Int {
    return a + b;
}


// Invalid:

fn Add(a: Int, b: Int) -> Int {
    return a + b;
}

module Math;


// A file without a module declaration cannot be imported.


// ============================================================
// Module names and file names
// ============================================================

// The module name does not have to match the file name.


// File: MathUtils.xs

module Math;


// This is valid.


// Matching the module and file name is recommended:

// File: Math.xs

module Math;


// imports resolves the module name, not the file name:

imports Math;


// The following does not depend on the source file being named
// Math.xs.


// ============================================================
// Unique module names
// ============================================================

// The same module name cannot be declared by multiple files
// in the same project/module resolution scope.


// Invalid:

// File: A.xs

module Math;


// File: B.xs

module Math;


// Duplicate module declarations produce a compile-time error.


// ============================================================
// Import behavior
// ============================================================

imports Math;


// imports:
//
// - Resolves a module by its declared module name.
// - Makes module declarations accessible.
// - Does not copy source text.
// - Does not behave like #include.
// - Does not directly add the imported file to addFiles.
// - Requires the target file to declare a module.


// Direct project inclusion and import resolution are separate:
//
// addFiles
//     Controls direct project build-graph inputs.
//
// imports
//     Controls access to declarations from named modules.


// ============================================================
// Project-system summary
// ============================================================

//
// .xsproj
//     Project manifest.
//
// addFiles.entry
//     Explicit entry source.
//
// addFiles list
//     Additional direct build-graph sources.
//
// entry: None with a non-empty list
//     First listed source becomes entry.
//
// entry: None with no listed source
//     No output is produced.
//
// imports
//     Resolves module declarations without textual inclusion.
//
// module
//     Optional for direct sources.
//     Required for importable sources.
//     Must be the first declaration when present.
//
// module name
//     Independent from the file name.
//     Must be unique.
//
