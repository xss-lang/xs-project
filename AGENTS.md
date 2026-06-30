# xs-project agent rules

Bu dosya `/home/hasan/Projeler/XS` monorepo kökü için geçerlidir. Depo, LLVM-project tarzı tek checkout modelinde
ilerler: kök dizin orkestrasyon alanıdır; gerçek projeler sibling dizinlerde yaşar.

## Monorepo yapısı

- Kök depo mantığı `xs-project`tir.
- Üst seviye CMake project adı `xs_project`tir.
- Build edilebilir CMake projeleri:
  - `xs`: X# derleyicisi.
  - `xsproj`: public C23 `.xsproj` manifest parser/lexer/model API’si.
- Future projeler:
  - `xsfmt`: Rust nightly + Serde tabanlı formatter.
  - `xstidy`: Rust nightly + Serde tabanlı linter.
  - `xs-analyzer`: Rust language server + TypeScript VS Code extension.
  - `xs-backend`: future native XS Backend project.
- Future runtime:
  - `xsrt`: henüz kaynak dizini yok; `XS_ENABLE_RUNTIMES` registry’sinde future runtime olarak kalır.
- Eski kök `sources/` düzenine yeni dosya ekleme. Kök `include/` yalnız ortak public C API başlıkları içindir.

## Proje dizinleri

- `xs/`
  - X# derleyici projesidir.
  - C kaynakları `xs/sources/` altındadır.
  - `xs` projesine özel public/internal C header dosyaları `xs/include/` altındadır.
  - Yeni derleyici bileşenleri varsayılan olarak C23 ile burada yazılır.
- `include/`
  - Projeler arası ortak public C header dosyaları içindir.
  - `xs/` ve `xsproj/` tarafından ortak kullanılan `xs/diagnostic.h`, `xs/source.h` ve `xs/compiler_check.h` burada durur.
- `xsproj/`
  - `.xsproj` manifest parser/lexer/model projesidir.
  - C kaynakları `xsproj/sources/` altındadır.
  - Public API header’ı `xsproj/include/xs/project.h` içindedir.
  - `#include <xs/project.h>` üçüncü parti araçlar için public C23 API yüzeyidir.
- `xsfmt/`
  - Future formatter projesidir.
  - Rust nightly kullanır.
  - Serde kullanılabilir ve beklenen bağımlılıktır.
  - Rust kaynakları `xsfmt/sources/` altındadır.
- `xstidy/`
  - Future linter projesidir.
  - Rust nightly kullanır.
  - Serde kullanılabilir ve beklenen bağımlılıktır.
  - Rust kaynakları `xstidy/sources/` altındadır.
- `xs-analyzer/`
  - Future Rust language server ve TypeScript VS Code extension projesidir.
  - Rust language server kaynakları `xs-analyzer/sources/` altındadır.
  - TypeScript VS Code extension kaynakları `xs-analyzer/sources/extension/` altındadır.
  - Resmi olarak desteklenen tek IDE entegrasyonu Visual Studio Code'dur.
  - JetBrains için resmi eklenti planlanmaz.
- `xs-backend/`
  - Future native XS Backend projesidir.
  - Şimdilik yalnız iskelet dizindir; implementation’a girme.
  - C23 ağırlıklı olacaktır.
  - Rust yalnız somut teknik gerekçeyle isteğe bağlı ve izole kullanılabilir.
  - NASM `.asm`/`.inc` dosyaları bulunabilir; x86-only varsayım yapılmamalıdır.
  - Planlanan C kaynakları `xs-backend/sources/`, header dosyaları `xs-backend/include/`, optional assembly kaynakları
    `xs-backend/asm/` altında durur.
- `XS/`
  - X# dil belgeleri, syntax örnekleri ve örnek proje dosyalarıdır.
  - `XS/later/` artık aktif staging alanı değildir; yeni future proje dosyaları kendi proje köklerine eklenmelidir.

## Dil ve implementation dili

- C23 ana geliştirme dilidir.
- Derleyici ve public C API bileşenleri C23 ile yazılır.
- C içinde `#include <stdbool.h>` kullanma; C23 `bool` kullan.
- C ve header dosyaları 500 satırı aşmamalıdır.
- Modülerlik çok önemlidir; büyüyen dosyaları erken böl.
- Derleyici içine Rust yalnız somut teknik gerekçe varsa eklenebilir; bu durumda `xs/sources/rust/` altında izole kalmalı
  ve C API’yi bozmamalıdır.
- `xsfmt` ve `xstidy` bu kuralın istisnasıdır: onlar baştan Rust nightly + Serde future tool projeleridir.
- Bu projede `.sh`, `.bash`, `.zsh`, `.fish` veya benzeri shell script dilleri kalıcı proje artifact’i olarak kullanılmaz; shell script kullanımı yasaktır.
- Script/otomasyon için Java 21 source-file shebang ya da D + `rdmd` kullanılır.
- D tarafında varsayılan compiler `ldc2` kabul edilir; `gdc` kullanımı yasaktır.
- Script klasörü `scripts/` kabul edilir. D ile yazılan `scripts/d/` içine yazılır. Java ile yazılan `scripts/java/` içine yazılır


## Build sistemi ve toolchain

- CMake kullan; Meson kullanma.
- Clang/LLVM tooling ve LLD kullan.
- GNU’dan uzak dur:
  - GCC-only davranış ekleme.
  - GNU Make gerektirme.
  - `gnu23` gibi GNU C dialect kullanma.
  - `_GNU_SOURCE` ekleme.
  - GNU binutils’e bağımlı olma.
- NASM `.asm`/`.inc` dosyaları ancak isteğe bağlı ve taşınabilir olduklarında kullanılabilir.
- x86-only varsayım yapma; x86-64 ve ARM64 uyumluluğu önemlidir.
- `CMakeLists.txt` dosyasını mekanik formatlama. Kullanıcının değişikliklerini koruyarak küçük patch uygula.

## Monorepo CMake sözleşmesi

- `XS_ENABLE_PROJECTS` build edilecek projeleri seçer.
- `XS_ENABLE_PROJECTS=xs` derleyiciyi build eder.
- `XS_ENABLE_PROJECTS=xsproj` yalnız `.xsproj` public parser kütüphanesini ve ilgili testi build eder.
- `XS_ENABLE_PROJECTS=all` tüm stable projeleri seçer.
- `xsfmt`, `xstidy`, `xs-analyzer` ve `xs-backend` şu an future project olarak kayıtlıdır; seçilirlerse CMake bilinçli
  hata vermelidir.
- `XS_ENABLE_RUNTIMES` future runtime seçimleri içindir.
- `XS_ENABLE_RUNTIMES=xsrt` şu an bilinçli hata vermelidir.
- `xsproj` tek başına build edilirken LLVM package zorunlu olmamalıdır.

## Formatting ve statik analiz

- `.clang-format`, `.clangd` ve `.clang-tidy` C23/Clang düzeniyle uyumlu kalmalıdır.
- C macro formatını etkileyen proje macro’ları `.clang-format` içine eklenmelidir.
- `.clangd` include path’leri yeni monorepo düzenine göre `include`, `xs/include` ve `xsproj/include` kullanmalıdır.
- `.clang-tidy` C/C23 kontrolleri `xs/`, `xsproj/` ve `tests/` alanlarına odaklanmalıdır.
- Rust tool configleri kökte tutulmaz; `xsfmt/` ve `xstidy/` altında tutulur.
- TypeScript/VS Code extension configleri `xs-analyzer/` altında tutulur.
- `xsfmt`, `xstidy` ve ileride eklenecek developer tool projeleri için standart kullanıcı konfigürasyon formatı TOML'dır.
- TOML kararı tool configleri içindir; `.xsproj` manifest syntax'ı TOML'a dönüştürülmez.

## X# syntax ve `.xsproj`

- `.xsproj` dosyaları XS proje manifestleridir.
- `.xsproj` syntax’ını JSON, YAML, TOML veya XML ile değiştirme.
- `.xsproj` lexer/parser kodu `.xs` lexer/parser kodundan ayrıdır.
- `.xsproj` yalnız `//` ve `///` satır yorumlarını destekler.
- `.xsproj` multiline comment desteklemez.
- `compilerOptions.addFiles.entry` program entry source file’dır.
- `addFiles` içindeki diğer dosyalar ek source file’dır.
- Geçerli `appRelease` değerleri `ALPHA`, `BETA`, `STABLE`.
- `compilerOptions.xsBackend` değerleri `"LLVM"` ve `"XS"` olabilir.
- Desteklenmeyen backend davranışı sessizce tahmin edilmemeli; diagnostic üretilmelidir.
- X# syntax handling veya `XS/example` dosyaları değiştirilmeden önce ilgili `XS/Syntax/` dosyaları okunmalıdır.
- Belgelenmiş X# syntax, `.xsproj` ve XLIL biçimleri her zaman önceliklidir.
- Belgelenmemiş syntax, `.xsproj`, XLIL, semantik veya ABI biçimleri gerekiyorsa artık bekleme: Rust, TypeScript, C#,
  C23 ve hedef assembly convention’larından ilham alarak X# ile tutarlı biçimde tasarla.
- Uydurulan yeni biçim/karar ilgili dokümana eklenmelidir; belgelenmiş kurallarla çelişemez.

## Dil kararları

- `docs/TODO.md` artık soru listesi değil; X# v0 karar ve implementation takip dokümanıdır.
- Dil semantiği, runtime, ABI, MIR, backend ve tooling kararlarında `docs/TODO.md` ve `docs/IMPLEMENTATION.md` takip edilir.
- Belgelenmiş biçimler ve kararlar kaynak otoritedir; çelişki olduğunda mevcut doküman kazanır.
- Belgelenmemiş küçük veya büyük eksiklerde bekleme; kararı ver, `docs/TODO.md`, `docs/IMPLEMENTATION.md`, `XS/Syntax/`
  veya ilgili XLIL dokümanına yaz ve implementation’ı buna göre ilerlet.
- Tasarım ilhamı için Rust, TypeScript, C#, C23 ve hedef assembly convention’ları kullanılabilir.
- `byte` -> `u8`.
- `sbyte` -> `i8`.
- `char` 16-bit UTF-16 code unit’tir.
- `str` UTF-16’dır; uzunluğu keyfi compiler limitine değil runtime/platform representation sınırına bağlıdır.
- X# nominal typing kullanır.
- Kullanıcı tanımlı tip kimliği ad/symbol bazlıdır; aynı field yapısı tipleri uyumlu yapmaz.
- HIR dil tiplerini çözer.
- `bool`, HIR/low-level tip sınırında `i1` olur.
- HIR ve MIR LLVM API’lerine bağlı olmamalıdır.
- HIR ve MIR hedef bağımsız XLIL tip/veri sözlüğüne bağlı olabilir.
- Planlanan public API’ler:
  - `#include <xs/hir/jit.h>`: HIR baseline JIT.
  - `#include <xs/mir/jit.h>`: MIR performance JIT.
  - `#include <xs/lil.h>`: XLIL registry/generation API.
  - `#include <xs/lil/aot.h>`: XLIL AOT API.
- JIT katmanları uygulanmadan sahte JIT semantiği ekleme.

## Derleyici pipeline sırası

Belgelenmiş akışı sırayla takip et:

1. lexing ve parsing
2. structural AST
3. macro expansion
4. HIR
5. type checking
6. MIR
7. borrow checking
8. MIR optimization
9. monomorphization
10. codegen units
11. XLIL
12. LLVM IR veya future XS Backend lowering
13. optimization
14. object code
15. linking

Pipeline’ı atlayarak sahte semantic üretme. Eksik aşamada projeyi build edilebilir tut ve bir sonraki test edilebilir parçayı
ekle.

## CLI sözleşmesi

- Desteklenen komut biçimleri:
  - `xs check -proj MyApp.xsproj`
  - `xs build -proj MyApp.xsproj`
  - `xs run -proj MyApp.xsproj`
- Intermediate output seçenekleri:
  - `xs build --output hir -proj MyApp.xsproj`
  - `xs build --output mir -proj MyApp.xsproj`
  - `xs build --output xlil -proj MyApp.xsproj`
- Future direct XLIL build:
  - `xs build --xlil -file foo.xlil`
- `.xhir` HIR kodudur.
- `.xmir` MIR kodudur.
- `.xlil` XLIL kodudur.
- `.xlil` hiçbir zaman binary format olmayacaktır.

## Test ve doğrulama

- Her aşamayı build edilebilir ve test edilebilir bırak.
- Küçük patch tercih et; tek devasa compiler patch’i yapma.
- Touched subsystem için odaklı test çalıştır.
- Kullanıcının makinesinde parser/project testleri geçmişte OOM üretmiştir; test ve build komutlarında mümkünse 2GB limit
  kullan:
  - `ulimit -v 2097152`
- Pratik olduğunda çalıştır:
  - `cmake --build --preset clang-debug`
  - `ctest --preset clang-debug --output-on-failure`
  - `git diff --check`
- `xsproj` only değişikliklerinde ayrıca şu davranışı koru:
  - `XS_ENABLE_PROJECTS=xsproj` LLVM backend gerektirmeden configure/build olmalıdır.

## Git ve çalışma alanı

- Kullanıcının değişikliklerini koru.
- Kirli worktree olağandır; ilgisiz değişiklikleri geri alma.
- `git reset --hard`, `git checkout --` gibi destructive komutları açık izin olmadan kullanma.
- Generated artifact’leri repoya sokma:
  - `build/`
  - `target/`
  - `node_modules/`
  - `dist/`
  - `out/`
  - `Cargo.lock` future Rust tool iskeletleri için şimdilik ignore edilir.
