# X# v0 kararları ve uygulama takip listesi

Bu dosya artık “sorulacak büyük kararlar” listesi değildir. Buradaki kararlar X# v0 için uygulanacak sözleşmedir.
Eksik kalan işler karar eksikliği yüzünden değil, yalnızca implementation tamamlanmadığı için eksiktir.
Yeni kararlar kullanıcıdan ek belge beklemeden burada sabitlenir; mevcut belgelenmiş X# kuralları her zaman önceliklidir.
Eksik alanlarda Rust, TypeScript, C#, C23 ve hedef assembly geleneklerinden X# ile uyumlu ölçüde ilham alınabilir.

## Dil semantiği kararları

- Macro expansion sonrası declaration conflict çözümü AST üzerinde yapılır. Aynı namespace ve aynı declaration kind içindeki
  aynı isimler hatadır. `public`/dosya-yerel görünürlük conflict kuralını değiştirmez; görünürlük yalnız erişimi belirler.
- Macro expansion deterministiktir: dosya sırası, source order ve expansion depth sırasıyla kullanılır. Sonsuz recursion için
  sabit depth limiti diagnostic üretir.
- Lifetime kuralları Rust modelinden alınır: lexical region inference, outlives bound, higher-ranked lifetime bound ve elision
  kuralları Rust 2021 ile uyumlu olacak şekilde uygulanır. X# syntax farklı olduğunda aynı semantik AST alanlarına indirilir.
- X# nominal typing kullanır. User-defined tip kimliği ad/sembol üzerinden belirlenir; alanları ve yapısı aynı olan iki ayrı
  tip uyumlu kabul edilmez.
- Trait/interface uyumluluğu nominaldir. Bir tip yalnız açıkça ilgili interface’i implement ettiğinde uyumludur. Structural
  uyumluluk yalnız ayrı bir `structural` özelliği belgelenirse eklenecektir.
- Generic constraint doğrulaması nominal interface üyeliğine dayanır. Tüm constraint’ler conjunctive kabul edilir; herhangi
  biri sağlanmazsa instantiation hatadır.
- Overload seçim sırası: namespace/import visibility filtresi, arity filtresi, exact type match, coercion-free generic
  substitution, constraint doğrulaması, en spesifik aday seçimi. Eşit özgüllük ambiguity diagnostic üretir.
- Method merge, class inheritance zinciri boyunca nominal override kurallarıyla yapılır. Aynı imzada override için base
  declaration görünür olmalıdır; overload set’leri ayrı tutulur.
- Operator resolution önce primitive built-in operator set’ini, sonra visible user-defined operator set’ini dener. User-defined
  operator primitive exact match’i gölgeleyemez.
- `throws` function type’ın parçasıdır. Checked exception listesi HIR type-check sırasında doğrulanır, MIR terminator’lerinde
  normal ve exceptional edge olarak temsil edilir.
- `async fn` public olarak `Task<T>` döndürür; `T` return tipidir, `void` için `Task<void>` kullanılır. `await` yalnız awaitable
  trait/interface sözleşmesini sağlayan değerlerde geçerlidir.
- `Send` ve `Sync` auto-trait’tir. Primitive immutable değerler Send+Sync kabul edilir; raw runtime/resource handle tipleri
  açık implement gerektirir. Mutable aliasing borrow checker tarafından engellenir.
- Mutability place tabanlıdır. `mut` binding yeniden atama ve mutable borrow üretme yetkisi verir; immutable binding mutable
  borrow veremez.
- Drop sırası lexical scope içinde ters declaration sırasıdır. Partial initialization için yalnız initialize edilmiş field/value
  drop edilir.

## Runtime ve ABI kararları

- `char` 16-bit UTF-16 code unit’tir. Tek başına surrogate code unit taşıyabilir; scalar-value garantisi `char` seviyesinde
  verilmez.
- `str` UTF-16 code unit dizisidir. Uzunluk `size_t` code unit sayısıdır; uzunluk sınırı platform adres alanı ve `size_t` ile
  sınırlıdır. Null terminator zorunlu değildir.
- `str` v0 runtime layout’u `length: size_t`, `data: const uint16_t *`, `owner: void *` alanlarından oluşan fat handle’dır.
  `owner == NULL` borrowed/static string anlamına gelir.
- String literal normalization yalnız escape çözümleme yapar; Unicode canonical normalization yapmaz. Invalid escape diagnostic
  üretir; ham UTF-16 surrogate pair’leri korunur.
- `data` ve `class` field layout’u declaration order’a göre yapılır. Alignment ve padding hedef platform ABI’sine göre belirlenir.
- `class` değeri referans semantiğine sahiptir; object handle pointer-sized opaque reference olarak taşınır.
- `data` değeri value semantiğine sahiptir; field’ları inline taşınır.
- `enum` tag tipi varsayılan `u32`’dir. `enum data` layout’u `u32 tag` + en büyük payload için aligned union biçimindedir.
- Niche optimization v0’da yoktur. ABI kararlılığı için enum/data enum representation açık layout’a bağlı kalır.
- Interface dispatch v0’da itable kullanır: object header type descriptor’a, type descriptor interface id’den method table’a gider.
- Generic interface dispatch monomorfizasyon sonrası concrete itable ile yapılır.
- Cross-module public interface visibility module registry ve package metadata ile doğrulanır; private interface implement bilgisi
  dış modüle export edilmez.
- Exception ABI v0’da platform unwind desteği varsa zero-cost unwind, yoksa runtime personality abstraction kullanır. MIR cleanup
  edge’leri drop noktalarını taşır.
- `throw` payload’u runtime exception object reference’ıdır. `catch` nominal type match yapar.
- Async runtime ABI `Task<T>` header, state enum, poll function pointer, result storage ve exception slot içerir.
- Scheduler v0 work-stealing gerektirmez; platform thread pool abstraction yeterlidir. Cancellation cooperative flag ile yapılır.
- Thread/sync runtime C23 atomics ve platform native thread primitive’leri üzerinde ince soyutlama kullanır; GNU pthread uzantılarına
  bağlı değildir.

## MIR kararları

- MIR function, local table, basic block listesi ve terminator zorunluluğundan oluşur.
- Basic block tek girişli label’dır; block son instruction’dan sonra tam olarak bir terminator taşır.
- Value modeli typed SSA value id kullanır. Mutable memory için place/projection kullanılır.
- Place `local`, `field`, `deref`, `index` projection zinciridir.
- Terminator seti v0: `return`, `goto`, `switch`, `call`, `throw`, `unreachable`, `drop_then`.
- Exception yolları terminator üzerinde explicit exceptional target olarak tutulur.
- Borrow checker girdileri region id, loan id, move path, initialized path, drop flag ve alias set’lerinden oluşur.
- Borrow checker kuralı Rust NLL modeline yakındır: shared borrow çoklu olabilir, mutable borrow exclusive’dır, moved place okunamaz.
- Drop point doğrulaması MIR üzerinde yapılır; moved veya uninitialized value drop edilmez.
- MIR optimizasyon pass sırası: CFG cleanup, unreachable elimination, copy propagation, constant folding, dead store elimination,
  drop flag simplification, second CFG cleanup.
- MIR optimizasyonları observable drop order, exception edge ve borrow-check sonucu değiştiremez.

## Monomorfizasyon ve codegen kararları

- Monomorfizasyon lazy yapılır: reachable generic instantiation görüldükçe üretilir.
- Cache key `package-id`, `symbol-id`, canonical type arguments, target triple ve backend abi version’dan oluşur.
- Symbol naming stable mangling kullanır: `_XS` prefix, module path, symbol kind, generic argument hash.
- Aynı instantiation package boundary içinde paylaşılır; public generic instantiation cross-package cache’e yazılabilir.
- Codegen unit ayırma varsayılan olarak module path bazlıdır. Büyük module’ler function count eşiğine göre alt unit’e bölünebilir.
- Incremental cache content-addressed artifact store’dur; hash girdisi HIR/MIR canonical dump, target triple ve compiler version’dır.

## XLIL kararları

- XLIL backend’lerin ortak public giriş dilidir; HIR/MIR LLVM’e değil XLIL tip/veri sözlüğüne bağlı kalır.
- `.xlil` her zaman text registry formatıdır; binary XLIL formatı eklenmeyecektir.
- XLIL, CLR kadar high-level değildir ve assembly kadar low-level değildir; assembly’ye benzeyen ama hedef mimariden bağımsız
  bir orta-düşük seviye registry dilidir.
- XLIL registry dosyası `xlil module <name>` başlığıyla başlar ve declaration/definition kayıtları içerir.
- Function body modeli typed SSA, explicit basic block ve terminator yapısını kullanır.
- XLIL instruction set v0: constants, arithmetic, compare, load/store, address projection, call, branch, switch, return, throw,
  landingpad/cleanup, aggregate construct/extract.
- XLIL runtime ABI’ye özgü object layout’u tarif etmez; yalnız hedef bağımsız tip, sembol ve control-flow taşır.
- MIR → XLIL lowering borrow-check ve monomorfizasyon sonrası yapılır.
- XLIL → LLVM IR lowering target triple/data layout bilerek LLVM module/function/body üretir.
- XLIL public C23 API `#include <xs/lil.h>` altında AOT üretim, XLIL registry üretimi, modül, type, function, block,
  value ve instruction builder yüzeyleri sağlar.
- Üçüncü parti diller `xs/lil.h` üzerinden XLIL üreterek LLVM backend’i ve ileride XS Backend’i hedefleyebilir.

## JIT ve public ara katman API kararları

- HIR baseline JIT public C23 API hedefi `#include <xs/hir/jit.h>` başlığıdır.
- HIR baseline JIT hızlı geliştirme, denetim ve debug çalıştırması içindir; optimize native performans hedefi değildir.
- MIR performance JIT public C23 API hedefi `#include <xs/mir/jit.h>` başlığıdır.
- MIR performance JIT typed, borrow-check geçmiş ve optimizasyonlardan geçirilmiş MIR üzerinde çalışır.
- JIT API’leri HIR/MIR’i LLVM API’ye bağlamaz; gerekirse XLIL veya backend abstraction üzerinden hedefe iner.
- Bu API yüzeyleri üçüncü parti dil ve araçlar tarafından kullanılabilir olacak şekilde C23 public ABI olarak tasarlanır.
- JIT başlıkları ve davranışları implementation gelmeden sahte/stub semantik ile genişletilmeyecektir.

## Backend ve link kararları

- LLVM backend v0 birincil backend’tir. XS Backend seçeneği parse/model düzeyinde kabul edilir, implementation hazır olana kadar
  diagnostic ile reddedilir.
- Object output adı `<target-name>.o` veya platform object suffix’idir. Executable output proje `appName` değerinden türetilir.
- Linker abstraction shell çağırmaz; argv listesiyle çalışır.
- Linux hedefinde varsayılan linker `ld.lld` veya `clang --target=<triple> -fuse-ld=lld` yoludur; GNU binutils kullanılmaz.
- Runtime linkleme explicit XS runtime archive/shared object üzerinden yapılır.
- Object/link artifact dizini `build/xs/<profile>/<target>/` olarak belirlenir.

## Tooling ve proje akışı kararları

- `.xhir`, `.xmir`, `.xlil` text dump formatları deterministik, newline-normalized ve source-order stable olur.
- `xs check` parse, macro expansion, HIR, type-check ve borrow-independent semantic checks çalıştırır; object üretmez.
- `xs build` tüm check aşamalarını, MIR, borrow checker, monomorfizasyon, XLIL, backend ve link akışını çalıştırır.
- `xs run` önce `xs build` yapar, sonra executable’ı argument forwarding ile başlatır.
- `xs build --xlil -file <girdi.xlil>` proje manifesti okumadan XLIL parse, verify, backend ve link akışını çalıştırır.
- Başarılı komut exit code `0`, diagnostic error varsa `1`, internal compiler error varsa `70` döndürür.
- Diagnostic kodları `XS####` biçimindedir. Severity: note, warning, error, fatal.
- Makine-okunur diagnostic çıktı ileride `--diagnostic-format json` ile JSON Lines olarak sağlanır.
- `compilerOptions.xsBackend: "LLVM"` LLVM backend’i seçer; `"XS"` XS Backend hazır değilse feature diagnostic üretir.
- Public module/package dağıtımı manifest metadata, exported symbol table ve compiled interface summary içerir.
- Cross-project import çözümleme package name/version, target triple compatibility ve exported module path üzerinden yapılır.

## Uygulama durumu

- Kararlar yukarıda sabitlenmiştir; implementation bu sözleşmeye doğru aşamalı ilerleyecektir.
- Geçici veya belgelenmemiş ABI/API eklenmeyecek; yeni implementation bu dosyadaki kararlara ve `docs/IMPLEMENTATION.md` akışına
  bağlanacaktır.
