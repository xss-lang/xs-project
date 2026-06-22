// Macro system:

//{
The macro system provides AST-based declarative macros.

Macro expansion runs after AST creation and before HIR generation.

Macros are defined with macroRules!.

Macros:

- Match syntax fragments.
- Expand into AST nodes.
- Follow lexical scope rules.
- May be declared at top level or inside another scope.
- May be called before their textual declaration.
- May call other macros when no recursion cycle is created.
- Are not hygienic.
}//

imports Stdio;


// ============================================================
// Basic macro
// ============================================================

macroRules! say {
    ($x:expr): {
        std.cout << $x << "\n";
    };
}

fn Main() {
    say!("Alfa");
}


// Equivalent expansion:

fn ExpandedMain() {
    std.cout << "Alfa" << "\n";
}


// ============================================================
// Macro definition
// ============================================================

// General form:
//
// macroRules! macroName {
//     (matcher): {
//         expansion
//     };
// }


// macroRules!
//
// - Begins a macro definition.
// - Must be followed by the macro name.
// - The macro name follows normal identifier rules.
// - Each matcher is written inside parentheses.
// - Each expansion is written inside braces.
// - Every macro rule must end with semicolon.
// - The final rule must also end with semicolon.


// Valid:

macroRules! empty {
    (): {
    };
}


// Invalid because the rule does not end with semicolon:

macroRules! invalidRule {
    (): {
    }
}


// ============================================================
// Macro calls
// ============================================================

// Macro calls use ! followed by parentheses.

say!("Alfa");


// Only parentheses are valid for macro calls.


// Invalid:

say!["Alfa"];
say!{"Alfa"};


// A macro call used as a statement follows ordinary statement
// semicolon rules.


// ============================================================
// Multiple rules
// ============================================================

macroRules! output {
    (): {
        std.cout << "\n";
    };

    ($value:expr): {
        std.cout << $value << "\n";
    };
}


// A macro may contain multiple matcher rules.
//
// If no rule matches:
//
// - A compile-time error is produced.


// If multiple rules match:
//
// - Every matching rule is expanded.
// - Matching rules are expanded in declaration order.


// Example:

macroRules! describe {
    ($expression:expr): {
        std.cout << "expression\n";
    };

    ($identifier:ident): {
        std.cout << "identifier\n";
    };
}

fn MultipleMatchingRules() {
    describe!(value);
}


// If value matches both expr and ident, expansion order is:

fn ExpandedMultipleMatchingRules() {
    std.cout << "expression\n";
    std.cout << "identifier\n";
}


// ============================================================
// Fragment specifiers
// ============================================================

// Supported macro fragment specifiers:
//
// - expr
// - ident
// - ty
// - path
// - pat
// - stmt
// - block
// - item
// - literal
// - meta
// - tt
// - lifetime
// - vis


// Expression:

macroRules! expressionMacro {
    ($value:expr): {
        std.cout << $value;
    };
}


// Identifier:

macroRules! identifierMacro {
    ($name:ident): {
        $name: int = 42;
    };
}


// Type:

macroRules! typeMacro {
    ($typeValue:ty): {
        value: $typeValue;
    };
}


// Path:

macroRules! pathMacro {
    ($modulePath:path): {
        $modulePath;
    };
}


// Pattern:

macroRules! patternMacro {
    ($patternValue:pat): {
        match value {
            $patternValue -> {
            },
        };
    };
}


// Statement:

macroRules! statementMacro {
    ($statementValue:stmt): {
        $statementValue
    };
}


// Block:

macroRules! blockMacro {
    ($blockValue:block): {
        $blockValue
    };
}


// Item:

macroRules! itemMacro {
    ($itemValue:item): {
        $itemValue
    };
}


// Literal:

macroRules! literalMacro {
    ($literalValue:literal): {
        std.cout << $literalValue;
    };
}


// Metadata:

macroRules! metadataMacro {
    ($metadataValue:meta): {
        $metadataValue
    };
}


// Token tree:

macroRules! tokenTreeMacro {
    ($tokenValue:tt): {
        $tokenValue
    };
}


// Lifetime:

macroRules! lifetimeMacro {
    ($lifetimeValue:lifetime): {
        $lifetimeValue
    };
}


// Visibility:

macroRules! visibilityMacro {
    ($visibilityValue:vis): {
        $visibilityValue fn Generated() {
        }
    };
}


// ============================================================
// Repetition
// ============================================================

// Zero-or-more repetition:

// $(
//     ...
// )*


// One-or-more repetition:

// $(
//     ...
// )+


// Optional repetition with ? is not supported.


// Zero or more expressions:

macroRules! printMany {
    ($(
        $value:expr
    )*): {
        $(
            std.cout << $value << "\n";
        )*
    };
}


// One or more expressions:

macroRules! printAtLeastOne {
    ($(
        $value:expr
    )+): {
        $(
            std.cout << $value << "\n";
        )*
    };
}


// ============================================================
// Repetition separators
// ============================================================

// Comma is the only supported repetition separator.


// Zero or more comma-separated expressions:

macroRules! commaSeparated {
    ($(
        $value:expr
    ),*): {
        $(
            std.cout << $value << "\n";
        )*
    };
}


// One or more comma-separated expressions:

macroRules! commaSeparatedRequired {
    ($(
        $value:expr
    ),+): {
        $(
            std.cout << $value << "\n";
        )*
    };
}


// Valid:

fn RepetitionCalls() {
    commaSeparated!();
    commaSeparated!(1);
    commaSeparated!(1, 2, 3);

    commaSeparatedRequired!(1);
    commaSeparatedRequired!(1, 2, 3);
}


// The following separators are not supported:
//
// - semicolon
// - vertical bar
// - =>
// - any token other than comma


// ============================================================
// Macro variables
// ============================================================

// Macro variables begin with $.

macroRules! duplicateOutput {
    ($value:expr): {
        std.cout << $value << $value;
    };
}


// A captured macro variable may be used multiple times in the
// expansion body.


// A macro variable name may be declared only once in the entire
// macroRules! definition.


// Invalid: duplicate variable in one rule.

macroRules! invalidDuplicateInRule {
    ($value:expr, $value:expr): {
    };
}


// Invalid: duplicate variable across different rules.

macroRules! invalidDuplicateAcrossRules {
    ($value:expr): {
    };

    ($value:ident): {
    };
}


// Invalid: duplicate variable inside and outside repetition.

macroRules! invalidDuplicateWithRepetition {
    (
        $value:expr,
        $(
            $value:expr
        ),*
    ): {
    };
}


// Valid: every matcher variable has a unique name.

macroRules! uniqueVariables {
    ($expressionValue:expr): {
    };

    ($identifierValue:ident): {
    };
}


// ============================================================
// Scope
// ============================================================

// A macro may be declared at top level.

macroRules! globalMacro {
    (): {
        std.cout << "global\n";
    };
}


// A top-level macro is global.


// A macro may also be declared inside a scope.

fn LocalMacroScope() {
    macroRules! localMacro {
        (): {
            std.cout << "local\n";
        };
    }

    localMacro!();
}


// A scoped macro is available only in its declaring scope.


// Nested scopes may use a macro from an enclosing scope.

fn OuterScope() {
    macroRules! outerMacro {
        (): {
            std.cout << "outer\n";
        };
    }

    {
        outerMacro!();
    }
}


// A macro declared inside a scope is not available outside that
// scope.


// ============================================================
// Declaration order
// ============================================================

// A macro may be called before its textual declaration in the
// same scope.

fn CallBeforeDefinition() {
    localMacro!();

    macroRules! localMacro {
        (): {
            std.cout << "local\n";
        };
    }
}


// The compiler collects macro definitions in the relevant scope
// before expansion.


// ============================================================
// Generated declarations
// ============================================================

// Declarations produced by a macro become active in the scope in
// which the macro is called.

macroRules! createValue {
    (): {
        generatedValue: int = 42;
    };
}

fn GeneratedVariable() {
    createValue!();

    std.cout << generatedValue;
}


// Generated declarations may be used before the textual macro call
// because macro expansion is completed before later semantic stages.

macroRules! createType {
    (): {
        class GeneratedUser {
        }
    };
}

fn GeneratedTypeBeforeCall() {
    user: GeneratedUser = new();

    createType!();
}


// Generated declarations may include:
//
// - variables
// - functions
// - classes
// - enums
// - data declarations
// - other valid items


// ============================================================
// Macro hygiene
// ============================================================

// Macros are not hygienic.
//
// Identifiers created by an expansion are inserted directly into
// the call scope.


// Name conflicts produce compile-time errors.

macroRules! createConflictingValue {
    (): {
        value: int = 42;
    };
}

fn InvalidGeneratedNameConflict() {
    value: int = 1;

    createConflictingValue!();
}


// The expanded declaration conflicts with the existing value
// declaration in the same scope.


// ============================================================
// Macro-to-macro calls
// ============================================================

// A macro may call another macro.

macroRules! innerMacro {
    ($value:expr): {
        std.cout << $value << "\n";
    };
}

macroRules! outerMacro {
    ($outerValue:expr): {
        innerMacro!($outerValue);
    };
}

fn NestedMacroCall() {
    outerMacro!("Alfa");
}


// Macro call chains are allowed when they do not create a cycle.


// ============================================================
// Recursion
// ============================================================

// Direct macro recursion is forbidden.

macroRules! invalidDirectRecursion {
    (): {
        invalidDirectRecursion!();
    };
}


// Indirect macro recursion is also forbidden.

macroRules! invalidFirst {
    (): {
        invalidSecond!();
    };
}

macroRules! invalidSecond {
    (): {
        invalidFirst!();
    };
}


// The compiler constructs a macro call graph.
//
// Any direct or indirect cycle in the macro call graph is a
// compile-time error.


// ============================================================
// Expansion stage
// ============================================================

// Compiler flow:
//
// .xs source code
//     ↓
// Lexing + Parsing
//     ↓
// AST
//     ↓
// Macro expansion
//     ↓
// HIR


// Macro expansion operates directly on the AST.
//
// Expansion results are AST nodes.
//
// HIR generation receives the fully expanded AST.


// Macro expansion must complete before:
//
// - HIR generation
// - type checking
// - MIR generation
// - borrow checking
// - code generation


// ============================================================
// Invalid unmatched call
// ============================================================

macroRules! onlyLiteral {
    ($value:literal): {
        std.cout << $value;
    };
}

fn InvalidUnmatchedMacroCall() {
    onlyLiteral!(first + second);
}

// If the argument does not match any rule, compilation fails.


// ============================================================
// Summary
// ============================================================

//{
// Definition:
//
//     macroRules! name {
//         (matcher): {
//             expansion
//         };
//     }
//
// Call:
//
//     name!(arguments);
//
// Call delimiters:
//
//     () only
//
// Fragment specifiers:
//
//     expr
//     ident
//     ty
//     path
//     pat
//     stmt
//     block
//     item
//     literal
//     meta
//     tt
//     lifetime
//     vis
//
// Repetition:
//
//     $(...)*
//     $(...)+
//     $(...),*
//     $(...),+
//
// Separator:
//
//     comma only
//
// Scope:
//
//     top-level macro -> global
//     scoped macro    -> declaring scope only
//
// Rule matching:
//
//     no match         -> compile-time error
//     multiple matches -> expand all in declaration order
//
// Recursion:
//
//     direct recursion   -> forbidden
//     indirect recursion -> forbidden
//
// Hygiene:
//
//     macros are not hygienic
//     generated names enter the call scope
//}//
