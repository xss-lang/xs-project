// SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
// SPDX-License-Identifier: Apache-2.0

// Structural AST system:

//{
The structural AST represents parsed X# source code as explicit
syntax nodes instead of storing complete declarations and bodies only
as raw source ranges.

The parser constructs structural AST nodes.

The structural AST does not perform:

- Name resolution
- Import resolution
- Type checking
- Send or Sync validation
- Ownership validation
- Borrow checking
- Method resolution
- Operator resolution
- Monomorphization
- Code generation

Those operations belong to later compiler stages.
}//


// ============================================================
// Compiler position
// ============================================================

// .xs source code
//     ↓
// Lexing + Parsing
//     ↓
// Structural AST
//     ↓
// Macro expansion
//     ↓
// HIR
//     ↓
// Type checking
//     ↓
// MIR


// Macro declarations and macro calls are represented in the AST.
//
// Macro expansion operates directly on structural AST nodes.
//
// HIR receives the fully expanded AST.


// ============================================================
// Source spans
// ============================================================

// Every AST node stores its source location.

data SourceSpan {
    fileId: UInt
    startOffset: UInt
    endOffset: UInt
    startLine: UInt
    startColumn: UInt
    endLine: UInt
    endColumn: UInt
}


// SourceSpan is used for:
//
// - Diagnostics
// - Error highlighting
// - Macro expansion locations
// - Dependency tracking
// - Debug information
// - Source mapping


// ============================================================
// Root AST
// ============================================================

data AstFile {
    moduleDeclaration: ModuleDeclaration
    imports: ImportDeclaration[]
    declarations: Declaration[]
    span: SourceSpan
}


// moduleDeclaration may be None.
//
// A file without a module declaration may still be a direct
// build-graph source.
//
// A file that is imported must declare a module.


// ============================================================
// Declaration node family
// ============================================================

enum data Declaration {
    Module: ModuleDeclaration,
    Import: ImportDeclaration,
    Namespace: NamespaceDeclaration,
    Function: FunctionDeclaration,
    Class: ClassDeclaration,
    Interface: InterfaceDeclaration,
    Enum: EnumDeclaration,
    Data: DataDeclaration,
    Variable: VariableDeclaration,
    Macro: MacroDeclaration,
}


// ============================================================
// Module declarations
// ============================================================

data ModuleDeclaration {
    name: PathNode
    visibility: VisibilityNode
    span: SourceSpan
}


// module must be the first declaration when present.


// ============================================================
// Import declarations
// ============================================================

data ImportDeclaration {
    kind: ImportKind
    modulePath: PathNode
    importedNames: ImportName[]
    alias: IdentifierNode
    span: SourceSpan
}


enum ImportKind {
    ModuleImport,
    SelectedImport,
    WildcardImport,
}


data ImportName {
    name: IdentifierNode
    alias: IdentifierNode
    span: SourceSpan
}


// alias may be None.


// ============================================================
// Namespace declarations
// ============================================================

data NamespaceDeclaration {
    path: PathNode
    visibility: VisibilityNode
    span: SourceSpan
}


// ============================================================
// Visibility
// ============================================================

enum VisibilityKind {
    Default,
    Public,
    Private,
    Protected,
    Internal,
}


data VisibilityNode {
    kind: VisibilityKind
    span: SourceSpan
}


// ============================================================
// Identifiers and paths
// ============================================================

data IdentifierNode {
    text: Str
    span: SourceSpan
}


data PathNode {
    segments: IdentifierNode[]
    span: SourceSpan
}


// Example:
//
// Http.Response.BodyHandlers

// PathNode
// ├── Http
// ├── Response
// └── BodyHandlers


// ============================================================
// Generic parameters
// ============================================================

data GenericParameter {
    name: IdentifierNode
    constraints: TypeNode[]
    span: SourceSpan
}


// Example:
//
// T: Runnable, Printable

// GenericParameter
// ├── name: T
// └── constraints
//     ├── Runnable
//     └── Printable


// ============================================================
// Function declarations
// ============================================================

data FunctionDeclaration {
    name: IdentifierNode
    visibility: VisibilityNode

    isAsync: Bool
    isStatic: Bool
    isIncomplete: Bool

    genericParameters: GenericParameter[]
    parameters: ParameterDeclaration[]

    returnType: TypeNode
    throwsTypes: TypeNode[]

    body: BlockStatement
    span: SourceSpan
}


// returnType may be None when the function has no explicit
// `=> Type` declaration.
//
// A function without an explicit return type returns no value.


// ============================================================
// Parameters
// ============================================================

data ParameterDeclaration {
    name: IdentifierNode
    parameterType: TypeNode
    span: SourceSpan
}


// ============================================================
// Class declarations
// ============================================================

data ClassDeclaration {
    name: IdentifierNode
    visibility: VisibilityNode

    isIncomplete: Bool

    genericParameters: GenericParameter[]

    extendsTypes: TypeNode[]
    implementsTypes: TypeNode[]

    members: ClassMember[]
    span: SourceSpan
}


enum data ClassMember {
    Field: FieldDeclaration,
    Method: FunctionDeclaration,
    Constructor: ConstructorDeclaration,
    Destructor: DestructorDeclaration,
    NestedClass: ClassDeclaration,
    NestedEnum: EnumDeclaration,
    NestedData: DataDeclaration,
    NestedMacro: MacroDeclaration,
}


// ============================================================
// Fields
// ============================================================

data FieldDeclaration {
    name: IdentifierNode
    fieldType: TypeNode
    initializer: Expression
    visibility: VisibilityNode
    isStatic: Bool
    isImmutable: Bool
    span: SourceSpan
}


// initializer may be None.


// ============================================================
// Constructors
// ============================================================

data ConstructorDeclaration {
    className: IdentifierNode
    parameters: ParameterDeclaration[]
    body: BlockStatement
    visibility: VisibilityNode
    span: SourceSpan
}


// A class may contain at most one constructor.


// ============================================================
// Destructors
// ============================================================

data DestructorDeclaration {
    className: IdentifierNode
    body: BlockStatement
    span: SourceSpan
}


// Source form:
//
// File.Drop() {
// }


// ============================================================
// Interface declarations
// ============================================================

data InterfaceDeclaration {
    name: IdentifierNode
    visibility: VisibilityNode
    genericParameters: GenericParameter[]
    members: FunctionDeclaration[]
    span: SourceSpan
}


// ============================================================
// Enum declarations
// ============================================================

data EnumDeclaration {
    name: IdentifierNode
    visibility: VisibilityNode
    isDataEnum: Bool
    variants: EnumVariant[]
    span: SourceSpan
}


data EnumVariant {
    name: IdentifierNode
    payloadType: TypeNode
    span: SourceSpan
}


// payloadType may be None.
//
// Regular enum variants do not carry values.
//
// enum data variants may carry at most one payload.


// Example:
//
// enum data Token {
//     Identifier: Str,
//     Integer: Int,
//     Plus,
// }

// EnumDeclaration
// ├── Identifier -> Str
// ├── Integer    -> Int
// └── Plus       -> None


// ============================================================
// Data declarations
// ============================================================

data DataDeclaration {
    name: IdentifierNode
    visibility: VisibilityNode
    genericParameters: GenericParameter[]
    fields: DataField[]
    span: SourceSpan
}


data DataField {
    name: IdentifierNode
    fieldType: TypeNode
    span: SourceSpan
}


// data declarations contain fields only.


// ============================================================
// Variable declarations
// ============================================================

enum VariableBindingKind {
    Mutable,
    Immutable,
    Constant,
    StaticConstant,
}


data VariableDeclaration {
    name: IdentifierNode
    variableType: TypeNode
    initializer: Expression
    bindingKind: VariableBindingKind
    span: SourceSpan
}


// ============================================================
// Type node family
// ============================================================

enum data TypeNode {
    Named: NamedTypeNode,
    Generic: GenericTypeNode,
    Array: ArrayTypeNode,
    FixedArray: FixedArrayTypeNode,
    Pointer: PointerTypeNode,
    Reference: ReferenceTypeNode,
    MutableReference: MutableReferenceTypeNode,
    Tuple: TupleTypeNode,
    Function: FunctionTypeNode,
    Unit: UnitTypeNode,
}


data NamedTypeNode {
    path: PathNode
    span: SourceSpan
}


data GenericTypeNode {
    baseType: TypeNode
    arguments: TypeNode[]
    span: SourceSpan
}


data ArrayTypeNode {
    elementType: TypeNode
    span: SourceSpan
}


data FixedArrayTypeNode {
    elementType: TypeNode
    maximumIndex: Expression
    span: SourceSpan
}


data PointerTypeNode {
    pointeeType: TypeNode
    span: SourceSpan
}


data ReferenceTypeNode {
    referencedType: TypeNode
    lifetime: LifetimeNode
    span: SourceSpan
}


data MutableReferenceTypeNode {
    referencedType: TypeNode
    lifetime: LifetimeNode
    span: SourceSpan
}


data TupleTypeNode {
    elementTypes: TypeNode[]
    span: SourceSpan
}


data FunctionTypeNode {
    parameterTypes: TypeNode[]
    returnType: TypeNode
    span: SourceSpan
}


data UnitTypeNode {
    span: SourceSpan
}


// Example:
//
// Arc<Mutex<Str>>
// fn(Int, Str) => Bool

// GenericTypeNode
// ├── baseType: Arc
// └── arguments
//     └── GenericTypeNode
//         ├── baseType: Mutex
//         └── arguments
//             └── NamedTypeNode(Str)


// ============================================================
// Lifetimes
// ============================================================

data LifetimeNode {
    name: IdentifierNode
    span: SourceSpan
}


// Example:
//
// &'a User
// &'a mut User
// &'static Str
// &'_ User


// ============================================================
// Statement node family
// ============================================================

enum data Statement {
    Block: BlockStatement,
    Expression: ExpressionStatement,
    Variable: VariableDeclarationStatement,
    Return: ReturnStatement,
    If: IfStatement,
    For: ForStatement,
    ForEach: ForEachStatement,
    While: WhileStatement,
    Match: MatchStatement,
    Break: BreakStatement,
    Continue: ContinueStatement,
    Try: TryStatement,
    Throw: ThrowStatement,
    MacroCall: MacroCallStatement,
}


// ============================================================
// Block statements
// ============================================================

data BlockStatement {
    statements: Statement[]
    span: SourceSpan
}


// ============================================================
// Expression statements
// ============================================================

data ExpressionStatement {
    expression: Expression
    span: SourceSpan
}


// ============================================================
// Variable declaration statements
// ============================================================

data VariableDeclarationStatement {
    declaration: VariableDeclaration
    span: SourceSpan
}


// ============================================================
// Return statements
// ============================================================

data ReturnStatement {
    value: Expression
    span: SourceSpan
}


// value may be None for:
//
// return;


// ============================================================
// If statements
// ============================================================

data IfStatement {
    condition: Expression
    thenBlock: BlockStatement
    elseIfBranches: ElseIfBranch[]
    elseBlock: BlockStatement
    span: SourceSpan
}


data ElseIfBranch {
    condition: Expression
    body: BlockStatement
    span: SourceSpan
}


// elseBlock may be None.


// ============================================================
// For statements
// ============================================================

data ForStatement {
    initializer: Statement
    condition: Expression
    increment: Expression
    body: BlockStatement
    span: SourceSpan
}


// ============================================================
// For-each statements
// ============================================================

data ForEachStatement {
    pattern: PatternNode
    iterable: Expression
    body: BlockStatement
    span: SourceSpan
}


// Example:
//
// for (item in users.iter()) {
// }


// ============================================================
// While statements
// ============================================================

data WhileStatement {
    condition: Expression
    body: BlockStatement
    span: SourceSpan
}


// ============================================================
// Match statements
// ============================================================

data MatchStatement {
    value: Expression
    arms: MatchArm[]
    span: SourceSpan
}


data MatchArm {
    pattern: PatternNode
    body: Statement
    span: SourceSpan
}


// ============================================================
// Break and continue
// ============================================================

data BreakStatement {
    span: SourceSpan
}


data ContinueStatement {
    span: SourceSpan
}


// ============================================================
// Try statements
// ============================================================

data TryStatement {
    tryBlock: BlockStatement
    catches: CatchClause[]
    finallyBlock: BlockStatement
    span: SourceSpan
}


data CatchClause {
    variableName: IdentifierNode
    exceptionType: TypeNode
    body: BlockStatement
    span: SourceSpan
}


// finallyBlock may be None.


// ============================================================
// Throw statements
// ============================================================

data ThrowStatement {
    value: Expression
    span: SourceSpan
}


// ============================================================
// Expression node family
// ============================================================

enum data Expression {
    Identifier: IdentifierExpression,
    Literal: LiteralExpression,
    Binary: BinaryExpression,
    Unary: UnaryExpression,
    Assignment: AssignmentExpression,
    Call: CallExpression,
    MethodCall: MethodCallExpression,
    MemberAccess: MemberAccessExpression,
    Index: IndexExpression,
    New: NewExpression,
    Function: FunctionExpression,
    Await: AwaitExpression,
    Move: MoveExpression,
    Borrow: BorrowExpression,
    MutableBorrow: MutableBorrowExpression,
    Dereference: DereferenceExpression,
    ArrayLiteral: ArrayLiteralExpression,
    ObjectLiteral: ObjectLiteralExpression,
    FieldSet: FieldSetExpression,
    IoTarget: IoTargetExpression,
    Tuple: TupleExpression,
    MacroCall: MacroCallExpression,
}


// ============================================================
// Identifier expressions
// ============================================================

data IdentifierExpression {
    identifier: IdentifierNode
    span: SourceSpan
}


// ============================================================
// Literal expressions
// ============================================================

enum LiteralKind {
    Integer,
    FloatingPoint,
    String,
    Character,
    Boolean,
    None,
}


data LiteralExpression {
    kind: LiteralKind
    sourceText: Str
    span: SourceSpan
}


// sourceText preserves the original spelling.
//
// Examples:
//
// 1'000
// 3.141'592
// 1e-6
// "Alfa"
// 'A'
// true
// None


// ============================================================
// Binary expressions
// ============================================================

enum BinaryOperator {
    Add,
    Subtract,
    Multiply,
    Divide,
    Remainder,

    Equal,
    NotEqual,
    Greater,
    Less,
    GreaterEqual,
    LessEqual,

    LogicalAnd,
    LogicalOr,

    BitwiseAnd,
    BitwiseOr,
}


data BinaryExpression {
    left: Expression
    operator: BinaryOperator
    right: Expression
    span: SourceSpan
}


// Example:
//
// a + b

// BinaryExpression
// ├── left: IdentifierExpression(a)
// ├── operator: Add
// └── right: IdentifierExpression(b)


// ============================================================
// Unary expressions
// ============================================================

enum UnaryOperator {
    LogicalNot,
    Positive,
    Negative,
}


data UnaryExpression {
    operator: UnaryOperator
    operand: Expression
    span: SourceSpan
}


// ============================================================
// Assignment expressions
// ============================================================

data AssignmentExpression {
    target: Expression
    value: Expression
    span: SourceSpan
}


// ============================================================
// Function calls
// ============================================================

data CallExpression {
    callee: Expression
    arguments: Expression[]
    span: SourceSpan
}


// Example:
//
// Add(1, 2)


// ============================================================
// Method calls
// ============================================================

data MethodCallExpression {
    receiver: Expression
    methodName: IdentifierNode
    arguments: Expression[]
    span: SourceSpan
}


// Example:
//
// client.send(request, handler)


// MethodCallExpression
// ├── receiver: client
// ├── methodName: send
// └── arguments
//     ├── request
//     └── handler


// ============================================================
// Member access
// ============================================================

data MemberAccessExpression {
    receiver: Expression
    memberName: IdentifierNode
    span: SourceSpan
}


// ============================================================
// Index expressions
// ============================================================

data IndexExpression {
    receiver: Expression
    index: Expression
    span: SourceSpan
}


// ============================================================
// Object creation
// ============================================================

data NewExpression {
    constructedType: TypeNode
    arguments: Expression[]
    span: SourceSpan
}


// `constructedType` may be None when the source spelling is `new()`.
// In that form HIR resolves the constructed type from the assignment,
// argument or return context.


// ============================================================
// Function expressions
// ============================================================

data FunctionExpression {
    parameters: ParameterNode[]
    returnType: TypeNode
    body: BlockStatement
    captureKind: FunctionCaptureKind
    span: SourceSpan
}


enum FunctionCaptureKind {
    Default,
    Move,
}


// Examples:
//
// fn(value: Int) => Int {
//     return value + 1;
// }
//
// move fn() {
//     Thread.yield();
// }


// ============================================================
// Await expressions
// ============================================================

data AwaitExpression {
    task: Expression
    span: SourceSpan
}


// ============================================================
// Move expressions
// ============================================================

data MoveExpression {
    value: Expression
    span: SourceSpan
}


// ============================================================
// Borrow expressions
// ============================================================

data BorrowExpression {
    value: Expression
    span: SourceSpan
}


data MutableBorrowExpression {
    value: Expression
    span: SourceSpan
}


// ============================================================
// Dereference expressions
// ============================================================

data DereferenceExpression {
    value: Expression
    span: SourceSpan
}


// ============================================================
// Array literals
// ============================================================

data ArrayLiteralExpression {
    elements: Expression[]
    span: SourceSpan
}


// ============================================================
// Object literals
// ============================================================

data ObjectLiteralExpression {
    fields: ObjectLiteralField[]
    span: SourceSpan
}


data ObjectLiteralField {
    name: IdentifierNode
    value: Expression
    span: SourceSpan
}


// ============================================================
// Data field set expressions
// ============================================================

data FieldSetExpression {
    fieldName: IdentifierNode
    value: Expression
    span: SourceSpan
}


// Example:
//
// set.name{"Alfa"}


// ============================================================
// I/O target expressions
// ============================================================

data IoTargetExpression {
    target: Expression
    span: SourceSpan
}


// Example:
//
// std.fout << [stdout] << "Hello\n"


// ============================================================
// Tuple expressions
// ============================================================

data TupleExpression {
    elements: Expression[]
    span: SourceSpan
}


// ============================================================
// Pattern node family
// ============================================================

enum data PatternNode {
    Identifier: IdentifierPattern,
    Literal: LiteralPattern,
    EnumVariant: EnumVariantPattern,
    Tuple: TuplePattern,
    Wildcard: WildcardPattern,
    Else: ElsePattern,
}


data IdentifierPattern {
    identifier: IdentifierNode
    span: SourceSpan
}


data LiteralPattern {
    literal: LiteralExpression
    span: SourceSpan
}


data EnumVariantPattern {
    path: PathNode
    bindings: PatternNode[]
    span: SourceSpan
}


data TuplePattern {
    elements: PatternNode[]
    span: SourceSpan
}


data WildcardPattern {
    span: SourceSpan
}


data ElsePattern {
    span: SourceSpan
}


// ============================================================
// Macro declarations
// ============================================================

data MacroDeclaration {
    name: IdentifierNode
    rules: MacroRule[]
    span: SourceSpan
}


data MacroRule {
    matcher: MacroMatcher
    expansion: MacroExpansion
    span: SourceSpan
}


// ============================================================
// Macro matchers
// ============================================================

data MacroMatcher {
    elements: MacroMatcherElement[]
    span: SourceSpan
}


enum data MacroMatcherElement {
    Token: MacroTokenMatcher,
    Fragment: MacroFragmentMatcher,
    Repetition: MacroRepetitionMatcher,
}


data MacroTokenMatcher {
    tokenText: Str
    span: SourceSpan
}


data MacroFragmentMatcher {
    variableName: IdentifierNode
    fragmentKind: MacroFragmentKind
    span: SourceSpan
}


enum MacroFragmentKind {
    Expr,
    Ident,
    Ty,
    Path,
    Pat,
    Stmt,
    Block,
    Item,
    Literal,
    Meta,
    TokenTree,
    Lifetime,
    Visibility,
}


enum MacroRepetitionKind {
    ZeroOrMore,
    OneOrMore,
}


data MacroRepetitionMatcher {
    elements: MacroMatcherElement[]
    separator: MacroSeparator
    repetitionKind: MacroRepetitionKind
    span: SourceSpan
}


enum MacroSeparator {
    None,
    Comma,
}


// Only comma may be used as a repetition separator.


// ============================================================
// Macro expansions
// ============================================================

data MacroExpansion {
    elements: MacroExpansionElement[]
    span: SourceSpan
}


enum data MacroExpansionElement {
    Token: MacroExpansionToken,
    Variable: MacroExpansionVariable,
    Repetition: MacroExpansionRepetition,
}


data MacroExpansionToken {
    tokenText: Str
    span: SourceSpan
}


data MacroExpansionVariable {
    variableName: IdentifierNode
    span: SourceSpan
}


data MacroExpansionRepetition {
    elements: MacroExpansionElement[]
    repetitionKind: MacroRepetitionKind
    span: SourceSpan
}


// ============================================================
// Macro calls
// ============================================================

data MacroCallExpression {
    macroName: IdentifierNode
    arguments: MacroArgument[]
    span: SourceSpan
}


data MacroCallStatement {
    call: MacroCallExpression
    span: SourceSpan
}


data MacroArgument {
    tokens: TokenNode[]
    span: SourceSpan
}


// Macro calls use parentheses only.


// ============================================================
// Token preservation
// ============================================================

data TokenNode {
    kind: TokenKind
    sourceText: Str
    span: SourceSpan
}


// TokenNode is used where exact syntax must remain available,
// especially during macro matching and expansion.


// ============================================================
// Parser responsibilities
// ============================================================

// The parser must:
//
// - Construct structural declaration nodes.
// - Parse parameter lists.
// - Parse generic parameter lists and constraints.
// - Parse type syntax.
// - Parse enum variants.
// - Parse class and interface members.
// - Parse statements.
// - Parse expressions.
// - Parse patterns.
// - Parse macro declarations.
// - Parse macro calls.
// - Attach SourceSpan to every node.
// - Preserve source token spelling when required.


// ============================================================
// Parser non-responsibilities
// ============================================================

// The parser must not:
//
// - Resolve names.
// - Resolve modules.
// - Search imported module files.
// - Check whether a type exists.
// - Check whether an identifier is visible.
// - Infer expression types.
// - Resolve method calls.
// - Resolve operators.
// - Validate ownership.
// - Validate borrows.
// - Validate Send or Sync.
// - Perform macro expansion.
// - Perform HIR lowering.
// - Perform MIR lowering.


// ============================================================
// HIR responsibilities
// ============================================================

// HIR receives expanded AST and performs:
//
// - Import resolution
// - Module resolution
// - Namespace resolution
// - Name resolution
// - Method-call normalization
// - Operator-call normalization
// - High-level syntax normalization
// - Dependency-graph construction


// ============================================================
// Type-checking responsibilities
// ============================================================

// Type checking performs:
//
// - Type compatibility checks
// - Generic constraint checks
// - Visibility checks
// - Mutability checks
// - Send checks
// - Sync checks
// - Async and await checks
// - Statically provable synchronization checks


// ============================================================
// MIR responsibilities
// ============================================================

// MIR makes explicit:
//
// - Ownership
// - Moves
// - Borrows
// - Drop points
// - Destructor calls
// - Control-flow edges
// - Match branching
// - Exception unwinding
// - Async state machines


// ============================================================
// Complete example
// ============================================================

// Source:
//
// fn Add(a: Int, b: Int) => Int {
//     return a + b;
// }


// Structural AST:
//
// AstFile
// └── FunctionDeclaration
//     ├── name: Add
//     ├── parameters
//     │   ├── ParameterDeclaration
//     │   │   ├── name: a
//     │   │   └── type: Int
//     │   └── ParameterDeclaration
//     │       ├── name: b
//     │       └── type: Int
//     ├── returnType: Int
//     └── body
//         └── BlockStatement
//             └── ReturnStatement
//                 └── BinaryExpression
//                     ├── left: a
//                     ├── operator: Add
//                     └── right: b


// ============================================================
// Implementation guidance for C23
// ============================================================

// The C23 compiler implementation may represent node families
// using tagged unions.


// Conceptual example:
//
// typedef enum {
//     XS_EXPR_IDENTIFIER,
//     XS_EXPR_LITERAL,
//     XS_EXPR_BINARY,
//     XS_EXPR_CALL,
//     XS_EXPR_METHOD_CALL,
//     XS_EXPR_AWAIT,
//     XS_EXPR_ASSIGNMENT
// } XsExprKind;
//
// typedef struct XsExpr XsExpr;
//
// struct XsExpr {
//     XsExprKind kind;
//     XsSourceSpan span;
//
//     union {
//         XsIdentifierExpr identifier;
//         XsLiteralExpr literal;
//         XsBinaryExpr binary;
//         XsCallExpr call;
//         XsMethodCallExpr method_call;
//         XsAwaitExpr await_expr;
//         XsAssignmentExpr assignment;
//     } as;
// };


// AST nodes should normally be allocated in compiler-owned arenas.
//
// AST child collections should use compiler vector structures.
//
// Every parser loop must guarantee token progress.
//
// If a parser iteration consumes no token:
//
// - Emit one diagnostic.
// - Recover by consuming a token or leaving the parse branch.
// - Do not repeatedly append diagnostics or AST nodes without
//   advancing.


// This progress rule prevents unbounded parser loops and memory
// growth.


// ============================================================
// Summary
// ============================================================

//{
// Structural AST:
//
//     Explicit syntax-node tree
//     SourceSpan on every node
//     No semantic resolution
//
// Major node groups:
//
//     AstFile
//     Declaration
//     Statement
//     Expression
//     TypeNode
//     PatternNode
//     MacroDeclaration
//     MacroCallExpression
//
// Parser:
//
//     Builds syntax structure
//
// Macro expansion:
//
//     Transforms AST into expanded AST
//
// HIR:
//
//     Resolves names, modules and high-level constructs
//
// Type checking:
//
//     Validates types and language constraints
//
// MIR:
//
//     Resolves ownership and execution behavior
//}//
