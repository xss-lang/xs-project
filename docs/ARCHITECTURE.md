# Derleyici mimarisi

X# derleyicisi aşamalı ve test edilebilir bir pipeline olarak geliştirilir. Her aşama kendi veri modelini üretir; sonraki
aşamalar tamamlanmamış semantiği backend içinde tahmin etmez.

## Pipeline

```text
.xs kaynakları
    → lexer
    → parser
    → structural AST
    → macro validation ve expansion view
    → HIR symbol/import/name/type resolution
    → expression/type checks
    → MIR
    → borrow checker
    → MIR optimization
    → monomorphization
    → codegen unit planning
    → XLIL
    → LLVM IR veya future XS Backend lowering
    → object emission
    → link
```

## Katman sınırları

### Frontend

Frontend `.xs` source text, token, parser ve structural AST katmanıdır. Parser syntax üretir; tamamlanmamış semantik için
LLVM veya MIR davranışı uydurmaz.

### Macro katmanı

Macro sistemi şu an validation, token expansion, synthetic reparse ve expanded view üretir. In-place AST replacement henüz
tamamlanmamıştır. HIR tüketicileri macro call replacement’larını expanded view API’leriyle okur.

### HIR

HIR şu işleri üstlenir:

- module/namespace/import çözümleme
- symbol table üretimi
- visibility kontrolü
- user-defined type ve primitive type resolution
- generic arity ve constraint çözümleme
- erken expression/mutability diagnostic’leri

HIR LLVM API’ye bağlı değildir. Tip bilgileri hedef bağımsız kalır ve gerekli olduğunda XLIL tip sözlüğüyle ilişki kurar.

### MIR

MIR control-flow, local/place/value ve terminator modelidir. Borrow checker ve MIR optimizer bu model üzerinde çalışır. MIR
henüz tam statement/expression lowering veya exception/async state machine üretimi yapmaz.

### XLIL

XLIL backend’lerin ortak giriş dilidir. `.xlil` text registry formatıdır ve binary olmayacaktır. Üçüncü parti frontend’ler
ileride `xs/lil.h` public C23 API’siyle XLIL üretebilir.

### Backend

LLVM backend frontend’den ayrıdır. LLVM context, target machine, module, data layout, function declaration lowering,
optimization pipeline, object emission ve linker invocation burada yaşar. HIR/MIR/XLIL başlıkları LLVM C API kavramı taşımaz.

## Tamamlanmamış büyük parçalar

- Tam AST macro replacement
- Genel expression type inference ve overload resolution
- Nominal interface üyelik doğrulaması
- Send/Sync, async/await ve checked exception type-check
- MIR statement/expression lowering
- Tam borrow checker region/loan/drop modeli
- Monomorfizasyon ve incremental cache
- XLIL → LLVM function body lowering
- `xs build` / `xs run` uçtan uca executable üretimi

Bu eksikler [IMPLEMENTATION.md](IMPLEMENTATION.md) ve [TODO.md](TODO.md) üzerinden takip edilir.
