# xs-project

`xs-project`, X# dili ve derleyici ailesi için LLVM-project tarzı monorepo köküdür. Bugünkü ana odak X# derleyicisi
(`xs`) ve üçüncü parti araçların kullanabileceği public `.xsproj` manifest parser/lexer/model API’sidir (`xsproj`).

Bu depo deneysel ama ciddi bir compiler çalışma alanıdır: her adım build edilebilir/test edilebilir kalmalı, mimari HIR/MIR
katmanlarını LLVM’e bağlamamalı ve belgelenmiş derleme akışı korunmalıdır.

## Hızlı başlangıç

Gereken ana araçlar:

- CMake
- Ninja
- Clang / LLVM araçları
- LLD
- BusyBox veya GNU dışı küçük coreutils alternatifleri önerilir

Varsayılan debug build:

```text
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

OOM geçmişi olan makinelerde test/build için 2GB sanal bellek limiti önerilir:

```text
ulimit -v 2097152
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

Örnek proje kontrolü:

```text
./build/clang-debug/xs check -proj XS/example/MyApp.xsproj
```

## Monorepo dizinleri

| Yol | Durum | Amaç |
| --- | --- | --- |
| `include/` | aktif | Projeler arası ortak public C başlıkları |
| `xs/` | aktif | X# derleyicisi, CLI, lexer/parser, AST, macro, HIR, MIR, XLIL, LLVM backend altyapısı |
| `xsproj/` | aktif | Public C23 `.xsproj` parser/lexer/model API’si |
| `Spec/` | aktif kaynak belge | X# syntax ve dil davranışı örnek/spec dosyaları |
| `docs/` | aktif belge | Mimari, build, CLI, backend, roadmap ve implementation durumu |
| `tests/` | aktif | C tabanlı birim ve integration testleri |
| `xsfmt/` | future | Rust nightly + Serde formatter |
| `xstidy/` | future | Rust nightly + Serde linter |
| `xs-analyzer/` | future | Rust language server + TypeScript VS Code extension |
| `xs-backend/` | future | C23 ağırlıklı native XS Backend |

## Derleyici pipeline’ı

Belgelenmiş sıra:

```text
.xs kaynakları
    → lexing ve parsing
    → structural AST
    → macro expansion
    → HIR ve dependency graph
    → type checking
    → MIR
    → borrow checker
    → MIR optimization
    → monomorphization
    → codegen units
    → XLIL
    → LLVM IR / future XS Backend lowering
    → optimization
    → object code
    → linking
```

Bu sıra yalnız doküman hedefi değildir; implementation kararlarında sınır çizgisidir. Frontend eksikleri backend içinde
uydurulmaz. HIR ve MIR LLVM API’ye bağlı değildir; backend giriş noktası hedef bağımsız XLIL katmanıdır.

## Şu an çalışan ana özellikler

- C23 tabanlı, Clang/LLVM odaklı build sistemi
- Anti-GNU CMake/toolchain kontrolleri
- `.xsproj` manifest parser/lexer/model public C API’si
- X# lexer ve structural AST parser
- Declaration/statement macro expansion için synthetic reparse ve expanded view altyapısı
- HIR symbol table, import/name/type çözümleme başlangıcı
- Primitive tip metadata ve nominal user-defined type resolution
- Literal initializer/assignment/return kontrolleri
- MIR model API, text writer, borrow-check iskeleti ve bazı MIR optimizasyonları
- XLIL model/writer ve sınırlı MIR → XLIL lowering çekirdeği
- LLVM context/module/target/object/link altyapısı

Güncel ayrıntılı durum için [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md) dosyasına bak.

## Sürüm politikası

Numaralı X# sürümleri LLVM IR üretimi çalışır hale geldikten sonra başlayacaktır. O zamana kadar kök
[CHANGELOG.md](CHANGELOG.md) dosyası `Unreleased` geliştirme günlüğü olarak tutulur.

## CLI özeti

Bugünkü desteklenen komutlar:

```text
xs check -proj MyApp.xsproj
xs build -proj MyApp.xsproj
xs build --output hir -proj MyApp.xsproj
xs build --output mir -proj MyApp.xsproj
xs build --output xlil -proj MyApp.xsproj
xs run -proj MyApp.xsproj
```

`xs build` ve `xs run` uçtan uca native executable üretimi tamamlanana kadar bazı yollar bilinçli diagnostic/failure
üretebilir. Ara çıktı uzantıları:

- `.xhir`: HIR text dump
- `.xmir`: MIR text dump
- `.xlil`: XLIL text registry

`.xlil` hiçbir zaman binary format olmayacaktır.

## Geliştirme kuralları

- Ana implementation dili C23’tür.
- Yeni/touched C kodunda `#include <stdbool.h>` kullanma; C23 `bool` kullan.
- Yeni/touched C kodunda `NULL` yerine `nullptr` tercih et.
- C ve header dosyaları 500 satırı aşmamalıdır.
- CMake kullanılır; Meson kullanılmaz.
- GNU C compiler, GNU Make, GNU binutils fallback’leri ve GNU C dialect’leri reddedilir.
- Kalıcı shell script yazılmaz; otomasyon için Java source-file veya D tercih edilir.
- Satır sayımı için `busybox wc` gibi GNU dışı araçları tercih et.
- Test/build sırasında OOM riskine karşı `ulimit -v 2097152` kullan.

Daha geniş katkı ve çalışma kuralları için [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) ve [.agents/AGENTS.md](.agents/AGENTS.md)
dosyalarına bak.

## Dokümantasyon haritası

- [docs/README.md](docs/README.md): dokümantasyon giriş noktası
- [docs/BUILDING.md](docs/BUILDING.md): build, test, toolchain ve OOM notları
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): derleyici mimarisi ve aşama sınırları
- [docs/CLI.md](docs/CLI.md): CLI sözleşmesi ve mevcut durum
- [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md): ayrıntılı implementation durumu
- [docs/TODO.md](docs/TODO.md): X# v0 kararları ve takip listesi
- [docs/MONOREPO.md](docs/MONOREPO.md): monorepo seçim modeli
- [docs/LLVM_BACKEND.md](docs/LLVM_BACKEND.md): LLVM backend altyapısı

## Lisans

Lisans ve notice bilgisi için kökteki `LICENSE.txt` ve `NOTICE.txt` dosyalarına bak.
