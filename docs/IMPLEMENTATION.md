# X# derleyicisinin mevcut durumu

Derleyici C23 ile yazılır; Clang, CMake, Ninja, LLVM araçları ve LLD kullanır. GNU C derleyicisi, GNU Make
üreteçleri, GNU binutils, GNU C lehçeleri ve `_GNU_SOURCE` kullanımı yapılandırma veya derleme aşamasında reddedilir.

Belgelenmiş derleme sırası korunur:

```text
.xs kaynakları
    → sözcüksel analiz ve ayrıştırma
    → yapısal AST
    → makro genişletme
    → HIR ve bağımlılık grafiği
    → tip denetimi
    → MIR
    → borrow checker
    → MIR optimizasyonu
    → monomorfizasyon
    → codegen unit ayırma
    → XLIL
    → LLVM IR ve LLVM optimizasyonu
    → nesne kodu
    → bağlama
```

## Tamamlanan altyapı

### XLIL bağlı orta katman kuralı

- HIR ve MIR LLVM’e bağlı değildir ve bağlı olmayacaktır.
- HIR ve MIR, hedef bağımsız XLIL tip/veri sözlüğüne bağlıdır.
- HIR/MIR başlıkları LLVM C API, target triple, data layout veya LLVM type/value kavramlarını içermez; XLIL başlıklarını
  kullanabilir.
- XLIL, HIR/MIR’in ortak düşük seviyeli sözlüğü ve backend’lerin ortak giriş noktası olarak LLVM IR’dan önce konumlanır.
- LLVM IR üretimi ayrı backend katmanıdır; HIR/MIR/XLIL LLVM C API kavramlarını taşımaz.
- Backend tüketicileri typed, borrow-checked ve monomorfize MIR hazır olduktan sonra MIR’i XLIL’e, oradan kendi hedef
  IR’larına indirir.
- LLVM şu an birincil backend odağıdır; fakat mimari Cranelift, C backend, interpreter veya başka hedefler eklenebilecek şekilde
  korunur.
- Hedefe özel assembly gerekirse ayrı backend/runtime katmanında tutulur. NASM `.asm`/`.inc` kullanımı serbesttir ama x86-64’e
  kilitlenemez; ARM64 uyumluluğu korunmalıdır.

### Anti-GNU yapı kuralı

- CMake yalnızca Clang, Ninja, LLVM binutils eşdeğerleri ve LLD hattını kabul eder.
- `llvm-ar`, `llvm-ranlib`, `llvm-nm`, `llvm-objcopy`, `llvm-objdump`, `llvm-strip` ve `ld.lld` dışındaki fallback araçlar
  reddedilir.
- GCC ve GNU Make generator reddedilir.
- Derleyici `-std=c23` ve `CMAKE_C_EXTENSIONS OFF` ile katı ISO C23 kipinde derlenir.
- `gnu23` gibi GNU C lehçeleri ve `_GNU_SOURCE` reddedilir.

### Proje sistemi

- `.xsproj` dosyaları özgün X# proje sözdizimiyle ayrıştırılır.
- Zorunlu alanlar, tekrar eden alanlar, bilinmeyen alanlar ve `appRelease` değerleri doğrulanır.
- `entry: nil` olduğunda belgelenmiş ilk ek kaynak seçimi uygulanır.
- Proje içindeki göreli yollar `.xsproj` dosyasının bulunduğu dizinden çözülür.
- `xs check -proj <proje.xsproj>` çalışır.
- `xs build --output hir|mir|xlil -proj <proje.xsproj>` seçenekleri tanınır.
- `.xhir`, `.xmir` ve `.xlil` resmi ara-kod çıktıları structural AST tamamlanana ve formatları belgelenene kadar
  üretilmez.
- `compilerOptions.xsBackend` opsiyonel olarak `"LLVM"` veya `"XS"` değerlerini kabul eder.
- `xsBackend` şimdilik yalnızca doğrulanıp proje modelinde saklanır; derleme akışını veya backend seçimini etkilemez.

### Lexer ve yapısal AST

- Belgelenmiş anahtar sözcükler, operatörler, yorumlar ve çok satırlı metinler tokenlaştırılır.
- ASCII tanımlayıcı kuralları uygulanır.
- Onluk tam sayılar, kayan noktalı sayılar, bilimsel gösterim ve `'` basamak ayırıcıları doğrulanır.
- String ve character literal kaynak yazımları AST literal düğümlerine taşınır; `char` değerinin 16-bit karakter
  olarak çözümlenmesi HIR tip aşamasına bırakılır.
- Parser arena tabanlı yapısal AST üretir.
- AST düğümleri dosya kimliği, offset, satır ve sütun içeren tam kaynak konumu taşır.
- Bildirim, tip, statement, expression, pattern ve makro düğüm aileleri temsil edilir.
- Named, generic, array, fixed array, pointer, reference, tuple, unit ve `fn(...) => T` function type düğümleri
  yapısal AST'de ayrıştırılır.
- Reference type içindeki lifetime yazımları Rust temel biçimleriyle (`&'a T`, `&'a mut T`, `&'static T`, `&'_ T`)
  `XS_SYNTAX_LIFETIME` düğümüyle AST'ye taşınır; lifetime elision ve doğrulama borrow checker aşamasına bırakılır.
- Fonksiyon parametreleri, dönüş tipi, `throws` tipleri ve fonksiyon gövdeleri yapısal düğümlerdir; ham gövde aralığı
  olarak saklanmaz.
- `data` declaration gövdeleri field-only olarak ayrıştırılır; method, constructor, destructor, inheritance ve interface
  üyeleri AST üretmeden diagnostic verir.
- Class constructor adı class adıyla eşleşmek zorundadır ve class başına en fazla bir constructor parser diagnostic'iyle
  doğrulanır.
- Interface declaration gövdeleri yalnızca gövdesiz function declaration imzaları kabul eder.
- Interface dışındaki gövdesiz function declaration sözdizimi `incomplete fn ...;` gerektirir; `incomplete fn` gövde
  içerirse parser diagnostic üretir.
- Regular enum variant'ları payload type içeremez; `enum data` en az bir typed variant gerektirir ve tuple payload
  parser diagnostic'iyle reddedilir.
- `fn(...) { ... }`, `fn(...) => T { ... }` ve `move fn(...) { ... }` function expression/closure biçimleri AST'de
  `XS_SYNTAX_EXPR_FUNCTION` düğümüyle temsil edilir; `move` capture ayrı AST bayrağıdır.
- `new()` object creation ifadesi AST'de `XS_SYNTAX_EXPR_NEW` olarak tutulur; constructed type kaynakta yazılmadığı
  durumlarda HIR tarafından bağlamdan çözülür.
- Data syntax içindeki `set.field{value}` AST'de `XS_SYNTAX_EXPR_FIELD_SET`, `value get.field` ise member access
  düğümü olarak tutulur.
- Stdio syntax içindeki `[target]` I/O hedefleri `XS_SYNTAX_EXPR_IO_TARGET` düğümüyle temsil edilir.
- `if`, `for`, for-each, `while`, `match`, `try`, `catch`, `finally`, `return`, `throw`, `break` ve `continue`
  yapısal olarak ayrıştırılır.

### Modül keşfi ve import grafiği

- Proje kökü, etkin `.xsproj` dosyasının bulunduğu dizindir.
- Proje kökü altındaki `.xs` dosyaları özyinelemeli olarak taranır.
- Modüller dosya adına göre değil, bildirilmiş tam modül yoluna göre kaydedilir.
- Aynı modül adının birden fazla dosyada bildirilmesi hata üretir.
- `imports` ve `froms ... imports ...` bağımlılıkları bildirilmiş modül adına göre çözülür.
- Bulunamayan import hedefleri hata üretir.
- Import edilen fakat `addFiles` içinde bulunmayan kaynaklar bağımlılık grafiğine alınır ve denetlenir.

Bu katman HIR dizini altında bulunur.

### HIR sembol toplama

- `xs check` akışı yapısal AST ve makro doğrulamadan sonra HIR sembol toplama aşamasını çalıştırır.
- Proje denetiminde direct kaynaklar ve import grafiğinden gelen kaynaklar ortak HIR sembol tablosuna alınır.
- Dosya içindeki `module` ve `namespace` bildirimleri etkin HIR namespace yolunu belirler.
- Dosyada `public namespace` kullanıldıysa, explicit visibility modifier taşımayan top-level declaration'lar HIR'da public
  kabul edilir.
- `public namespace`, explicit `private`, `internal` veya `protected` visibility modifier'larını ezmez.
- Üst seviye `fn`, `class`, `interface`, `enum`, `data` ve `macroRules!` bildirimleri sembol tablosuna alınır.
- Semboller kısa ad, namespace adı, tam nitelikli ad, görünürlük, kaynak konumu ve kaynak AST düğümünü taşır.
- Aynı namespace içinde aynı kısa ada sahip üst seviye bildirimler hata üretir.
- Aynı kısa ad farklı namespace altında kullanılabilir.
- `imports Module;` bildirimi import edilen modülün public üst seviye sembollerini modül nitelikli yerel adlarla açar.
- `froms Module imports Name;`, `froms Module imports Name as Alias;` ve `froms Module imports *;` bildirimi public
  üst seviye sembolleri yerel import kapsamına açar.
- Public olmayan semboller dış modül importuyla açılmaz.
- Fonksiyon gövdelerindeki doğrudan çağrı hedefleri, aynı namespace sembolleri ve import kapsamı üzerinden çözümlenir.
- Başka namespace/modül üzerinden tam nitelikli çağrı hedefleri yalnızca public sembollere çözümlenebilir.
- Aynı namespace içindeki private/internal semboller doğrudan qualified adla çözümlenebilir; dosya düzeyi module
  sınırı henüz ayrı metadata olarak HIR'a taşınmadığı için daha sıkı dosya/modül görünürlük ayrımı TODO'dur.
- İlk segmenti yerel parametre/değişken olan çağrı hedefleri tip denetimine ertelenir.

Bu aşama henüz metot/operator çözümleme, overload seçimi, generic constraint çözümleme veya tip tabanlı çağrı çözümleme
yapmaz.

### HIR tip çözümleme başlangıcı

- `xs check` akışı HIR import ve ad çözümlemeden sonra HIR tip çözümleme aşamasını çalıştırır.
- `XS/Syntax/Types.txt` içindeki primitive tip adları tanınır.
- `bool` HIR aşamasında 1 bit primitive olarak çözülür; LLVM backend bunu `i1` tipine indirir.
- `byte` unsigned 8 bit, `sbyte` signed 8 bit olarak HIR düzeyinde ayrı primitive tiplerdir.
- `char` 16 bit karakter tipidir.
- `str` UTF-16’dır ve uzunluğu UTF-16 temsilinin izin verdiği ölçüde sınırsız kabul edilir.
- HIR primitive metadata, runtime layout'u belgelenmiş primitive tipler için XLIL tip eşlemesini taşır.
- `str` runtime/ABI layout'u tamamlanmadığı için henüz XLIL tipine eşlenmez.
- Fonksiyon, data, enum, class/interface üyesi ve generic constraint içindeki tip adları doğrulanır.
- Generic parametre adları kendi declaration scope’unda tanınır.
- Kullanıcı tanımlı `class`, `interface`, `enum` ve `data` tipleri HIR sembol tablosu ve import kapsamı üzerinden çözülür.
- Başka namespace/modül üzerinden tam nitelikli tip kullanımları yalnızca public tip sembollerine çözümlenebilir.
- Generic tip kullanımlarında tip argümanı sayısı declaration’daki generic parametre sayısıyla eşleşmelidir.
- Generic type erasure olmadığı ve default generic parametre desteklenmediği için generic tiplerin argümansız kullanımı hata
  üretir.
- Generic constraint tipleri interface sembollerine çözümlenmelidir.

Bu aşama henüz expression type inference, overload seçimi, constraint üyelik/uyumluluk denetimi, trait/interface
uyumluluğu veya ABI/layout kararı üretmez.

### Makro doğrulama ve kapsam çözümleme

- Makro matcher değişkenleri, tekrar derinlikleri ve genişletme değişkenleri doğrulanır.
- Aynı scope içindeki doğrudan ve dolaylı makro recursion hatadır.
- Makro tanımları genişletme öncesinde ilgili scope için toplanır.
- Aynı scope içinde metinsel tanımdan önce makro çağrısı kabul edilir.
- İç scope’ta tanımlanan makro dış scope’tan çağrılamaz.
- Çağrıların görülebilir bir makro tanımına çözümlendiği denetlenir.
- Tam token matcher kuralları ve boş matcher kuralları eşleştirilebilir.

Tam fragment yakalama ve AST genişletme hâlâ tamamlanmamıştır; desteklenmeyen fragment matcher’lar için semantik
uydurulmaz.

### LLVM backend altyapısı

- Frontend’den ayrı `xs_backend_llvm` kütüphanesi vardır.
- LLVM context, target machine, target triple ve data layout yönetilir.
- Her codegen unit için ayrı LLVM module oluşturulur.
- Belgelenmiş sayısal primitive tipler LLVM tiplerine eşlenir.
- Gövdesiz fonksiyon bildirimi ve fonksiyon imzası indirgeme desteği vardır.
- `default<O0>`–`default<O3>` LLVM optimizasyon pipeline’ları yapılandırılabilir.
- LLVM module doğrulaması ve nesne dosyası üretimi çalışır.
- Linker, shell kullanılmadan ve argüman politikası üst katmana bırakılarak çağrılabilir.

Ayrıntılar: [LLVM_BACKEND.md](LLVM_BACKEND.md)

### XLIL hedefi

- `XS/XLIL.md`, X# için resmi düşük seviyeli ara dili XLIL olarak tanımlar.
- `.xhir` HIR kodu, `.xmir` MIR kodu ve `.xlil` XLIL kodu uzantısıdır; bu formatların resmi içeriği henüz
  belgelenmemiştir.
- XLIL, HIR/MIR'in bağlı olduğu hedef bağımsız tip/veri sözlüğüdür.
- XLIL, LLVM IR’dan önce ve backend’lerin ortak giriş noktası olacak şekilde tasarlanır.
- XLIL assembly değildir, bytecode değildir ve hedef mimariden bağımsız kalmalıdır.
- Kararlı C23 API hedefi `#include <xs/lil.h>` olarak belgelenmiştir.
- `xs/lil.h` altında hedef bağımsız XLIL modül, primitive tip ve gövdesiz fonksiyon bildirimi API iskeleti vardır.
- XLIL text writer yalnızca modül başlığı ve fonksiyon bildirimi yazar.

XLIL instruction set, function body modeli, runtime/ABI yerleşimi ve MIR → XLIL function body lowering hâlâ TODO’dur.

## Henüz tamamlanmayan aşamalar

- Makro fragment matcher eşleme motoru ve AST makro genişletmesi
- HIR metot ve operatör çözümleme
- Modül/import dışındaki tip, fonksiyon çağrısı, generic ve trait/interface bağımlılık kenarları
- Expression tip denetimi ve generic constraint doğrulaması
- Send, Sync, mutability ve async/await doğrulamaları
- MIR üretimi, exception yolları ve async state machine üretimi
- Borrow checker ve drop noktalarının doğrulanması
- MIR optimizasyonları
- Monomorfizasyon, codegen unit ayırma ve artımlı derleme önbelleği
- XLIL function body veri modeli ve MIR → XLIL body lowering
- XLIL’den LLVM IR’a fonksiyon gövdesi indirgeme
- Runtime/ABI yerleşimi tamamlanmamış `str` tipinin LLVM eşlemesi
- Üretilmiş nesne dosyalarının proje hedeflerine göre bağlanması
- `xs build` ve `xs run` komutlarının uçtan uca tamamlanması

Tamamlanmamış semantik aşamalar için parser veya LLVM backend içinde geçici dil kuralı üretilmez.

## Derleme ve test

```text
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug
```

Örnek projeyi denetlemek için:

```text
./build/clang-debug/xs check -proj XS/example/MyApp.xsproj
```
