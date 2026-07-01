# X# derleyicisinin mevcut durumu

Derleyici C23 ile yazılır; Clang, CMake, Ninja, LLVM araçları ve LLD kullanır. GNU C derleyicisi, GNU Make
üreteçleri, GNU binutils, GNU C lehçeleri ve `_GNU_SOURCE` kullanımı yapılandırma veya derleme aşamasında reddedilir.
Yeni ve dokunulan C kodunda `bool` doğrudan C23'ten alınır, `#include <stdbool.h>` kullanılmaz ve null pointer sabiti
olarak `NULL` yerine `nullptr` tercih edilir.

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

### Monorepo düzeni

- Depo LLVM-project benzeri monorepo seçimine geçirilmiştir.
- Monorepo kökü `xs-project`, üst seviye CMake project adı `xs_project` olarak düzenlenmiştir.
- Üst seviye CMake `XS_ENABLE_PROJECTS` değişkenini tanır; varsayılan kararlı proje `xs` olur.
- LLVM tarafındaki `LLVM_ENABLE_RUNTIMES` ayrımına paralel olarak `XS_ENABLE_RUNTIMES` eklenmiştir; bugün build edilebilir
  runtime yoktur.
- `XS_ENABLE_PROJECTS=all` tüm kararlı projeleri seçer.
- `xs` ve `xsproj` kararlı build edilebilir projelerdir.
- Kök `include/` dizini projeler arası ortak public C başlıkları için ayrılmıştır; `xs/include/` yalnız `xs`, `xsproj/include/`
  yalnız `xsproj` başlıkları içindir.
- `xsfmt` ve `xstidy` Rust nightly + Serde future project olarak, `xs-analyzer` Rust language server ve TypeScript VS Code
  extension future project olarak, `xs-backend` future native backend project olarak, `xsrt` future runtime olarak kayıtlıdır;
  henüz build’e alınmazlar.

### XLIL bağlı orta katman kuralı

- HIR ve MIR LLVM’e bağlı değildir ve bağlı olmayacaktır.
- HIR ve MIR, hedef bağımsız XLIL tip/veri sözlüğüne bağlıdır.
- HIR/MIR başlıkları LLVM C API, target triple, data layout veya LLVM type/value kavramlarını içermez; XLIL başlıklarını
  kullanabilir.
- XLIL, HIR/MIR’in ortak düşük seviyeli sözlüğü ve backend’lerin ortak giriş noktası olarak LLVM IR’dan önce konumlanır.
- LLVM IR üretimi ayrı backend katmanıdır; HIR/MIR/XLIL LLVM C API kavramlarını taşımaz.
- Backend tüketicileri typed, borrow-checked ve monomorfize MIR hazır olduktan sonra MIR’i XLIL’e, oradan kendi hedef
  IR’larına indirir.
- Planlanan HIR baseline JIT public API yüzeyi `#include <xs/hir/jit.h>`, MIR performance JIT public API yüzeyi
  `#include <xs/mir/jit.h>` ve XLIL AOT public API yüzeyi `#include <xs/lil/aot.h>` olarak ayrılır; bu API’ler HIR/MIR’i
  LLVM C API’ye bağımlı hale getirmez.
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
- `.xsproj` lexer, parser ve proje modeli uygulaması `xsproj/sources/` altında tutulur.
- `.xsproj` lexer/parser kodu `.xs` lexer/parser koduyla ortak değildir; yalnız `//` ve `///` satır yorumları desteklenir,
  multiline comment desteklenmez.
- `.xsproj` parser iç derleyici detayı değildir; üçüncü taraf araçların `.xsproj` dosyalarını JSON benzeri şekilde
  okuyabilmesi için public C23 API yüzeyi `#include <xs/project.h>` altında sağlanır.
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
- Top-level ve class member context’te `name!();` biçimli macro call’lar `XS_SYNTAX_DECL_MACRO_CALL` declaration düğümüyle
  yapısal AST’ye alınır. Bu düğüm item/declaration üreten macro expansion için giriş noktasıdır; üretilen item’ların
  scope’a gerçek AST replacement olarak eklenmesi sonraki makro genişletme adımıdır.
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
- Class `extends` bildirimi en fazla bir kez ve tek base class ile kullanılabilir; class `implements` bildirimi çoklu
  interface listesi kabul eder. Interface gövdesinde `extends` veya `implements` diagnostic üretir.
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
- `xs_hir_collect_symbols_expanded`, declaration macro expansion set verildiğinde `XS_SYNTAX_DECL_MACRO_CALL` düğümünün
  synthetic declaration reparse tree’sindeki üretilmiş declaration’ları macro call’ın bulunduğu etkin HIR namespace içinde
  sembol tablosuna alır. Duplicate symbol kontrolleri normal declaration’larla aynı namespace kuralını kullanır.
- `xs_hir_collect_member_symbols`, class/interface owner sembolü için ayrı HIR member symbol table üretir. V0 kapsamı field,
  field-benzeri macro çıktısı, method, constructor, destructor ve nested type üyeleridir.
- Method aynı adla tekrar edilirse X# method merge kuralına göre son declaration lookup sonucunda kazanır; field/nested member
  ad çakışmaları diagnostic üretir.
- Semboller kısa ad, namespace adı, tam nitelikli ad, görünürlük, kaynak konumu ve kaynak AST düğümünü taşır.
- Aynı namespace içinde aynı kısa ada sahip üst seviye bildirimler hata üretir.
- Aynı kısa ad farklı namespace altında kullanılabilir.
- `imports Module;` bildirimi import edilen modülün public üst seviye sembollerini modül nitelikli yerel adlarla açar.
- `froms Module imports Name;`, `froms Module imports Name as Alias;` ve `froms Module imports *;` bildirimi public
  üst seviye sembolleri yerel import kapsamına açar.
- Public olmayan semboller dış modül importuyla açılmaz.
- Fonksiyon gövdelerindeki doğrudan çağrı hedefleri, aynı namespace sembolleri ve import kapsamı üzerinden çözümlenir.
- Başka namespace/modül üzerinden tam nitelikli çağrı hedefleri yalnızca public sembollere çözümlenebilir.
- Public olmayan semboller yalnızca aynı namespace ve aynı kaynak dosya içinden doğrudan qualified adla çözümlenebilir.
- İlk segmenti yerel parametre/değişken olan çağrı hedefleri tip denetimine ertelenir.
- Explicit named type taşıyan local değişken/parametre üzerindeki `value.Method()` çağrıları HIR member symbol table üzerinden
  varlık açısından doğrulanır. Receiver tipi şu aşamada yalnız doğrudan identifier receiver ve named type annotation ile
  çözümlenir.
- `xs_hir_validate_name_uses_expanded`, statement macro replacement set verildiğinde doğrudan statement child listelerini
  `xs_macro_expand_child_statements` expanded view’i üzerinden dolaşır. Böylece macro expansion sonrası oluşan function/method
  call hedefleri HIR symbol/import scope içinde doğrulanır.

Bu aşama henüz metot/operator çözümleme, overload seçimi, generic constraint çözümleme veya tip tabanlı çağrı çözümleme
yapmaz. Member symbol table bu aşamalar için ilk HIR veri modelidir; method varlık doğrulaması dispatch, override veya
overload seçimi kararı vermez.

### HIR tip çözümleme başlangıcı

- `xs check` akışı HIR import ve ad çözümlemeden sonra HIR tip çözümleme aşamasını çalıştırır.
- `XS/Syntax/Types.txt` içindeki primitive tip adları tanınır.
- `bool` HIR aşamasında 1 bit primitive olarak çözülür; LLVM backend bunu `i1` tipine indirir.
- `byte` unsigned 8 bit, `sbyte` signed 8 bit olarak HIR düzeyinde ayrı primitive tiplerdir.
- `char` 16 bit karakter tipidir.
- `str` UTF-16’dır ve uzunluğu UTF-16 temsilinin izin verdiği ölçüde sınırsız kabul edilir.
- X# nominal typing kullanır; HIR type identity user-defined tiplerde ad/sembol kimliğine dayanır ve aynı structural shape
  otomatik uyumluluk sağlamaz.
- HIR primitive metadata, runtime layout'u belgelenmiş primitive tipler için XLIL tip eşlemesini taşır.
- `str` runtime/ABI layout'u tamamlanmadığı için henüz XLIL tipine eşlenmez.
- Fonksiyon, data, enum, class/interface üyesi ve generic constraint içindeki tip adları doğrulanır.
- Generic parametre adları kendi declaration scope’unda tanınır.
- Kullanıcı tanımlı `class`, `interface`, `enum` ve `data` tipleri HIR sembol tablosu ve import kapsamı üzerinden çözülür.
- Başka namespace/modül üzerinden tam nitelikli tip kullanımları yalnızca public tip sembollerine çözümlenebilir.
- Public olmayan tip sembolleri yalnızca aynı namespace ve aynı kaynak dosya içinden çözümlenebilir.
- Generic tip kullanımlarında tip argümanı sayısı declaration’daki generic parametre sayısıyla eşleşmelidir.
- Generic type erasure olmadığı ve default generic parametre desteklenmediği için generic tiplerin argümansız kullanımı hata
  üretir.
- Aynı declaration scope içinde aynı generic parametre adı tekrar tanımlanamaz.
- Generic constraint tipleri interface sembollerine çözümlenmelidir.
- Generic parametreler birden fazla constraint taşıyabilir; constraint listesindeki `, Identifier :` yeni generic parametre
  başlatır, aksi halde virgül aynı parametreye ek constraint ayırır.
- `xs_hir_resolve_types_expanded`, statement macro replacement set verildiğinde doğrudan statement child listelerini
  `xs_macro_expand_child_statements` expanded view’i üzerinden dolaşır. Böylece macro expansion sonrası oluşan type
  kullanımları da HIR symbol/import scope içinde doğrulanır.

Bu aşama henüz expression type inference, overload seçimi, constraint üyelik/uyumluluk denetimi, trait/interface
uyumluluğu veya ABI/layout kararı üretmez.

### Makro doğrulama ve kapsam çözümleme

- Makro matcher değişkenleri, tekrar derinlikleri ve genişletme değişkenleri doğrulanır.
- Aynı scope içindeki doğrudan ve dolaylı makro recursion hatadır.
- Makro tanımları genişletme öncesinde ilgili scope için toplanır.
- Macro fragment/token matcher yardımcıları `xs/sources/macro/fragment.c` içinde ayrılmıştır; validation, expansion
  traversal ve future fragment reparse desteği `xs/sources/macro/expansion.c` dosyasını büyütmeden genişletilmelidir.
- Aynı scope içinde metinsel tanımdan önce makro çağrısı kabul edilir.
- İç scope’ta tanımlanan makro dış scope’tan çağrılamaz.
- Çağrıların görülebilir bir makro tanımına çözümlendiği denetlenir.
- Tam token matcher kuralları ve boş matcher kuralları eşleştirilebilir.
- Tek token'la kesin doğrulanabilen `tt`, `ident`, `literal`, `lifetime` ve `vis` fragment matcher'ları çağrı token'larına
  göre eşleştirilir.
- Makro genişletme hazırlık API'si `xs_macro_prepare_expansion` olarak eklenmiştir. Bu aşama çağrıları scope içinde
  çözer, tek-token fragment veya tam-token matcher ile yapısal yeniden ayrıştırma gerektirmeden genişletilebilir çağrıları
  sayar, basit expansion token/substitution planı üretir ve `meta` fragmentı için genişletmeyi bilinçli olarak erteler.
- `expr`, `stmt`, `block`, `ty`, `path`, `item` ve `pat` fragment v0 desteği, validation ve expansion sırasında matcher içinde
  tek kalan fragment olduğu durumda çağrı parantezi içindeki token dizisini tek expression/statement/block/type/path/item/pattern
  fragment olarak yakalar. Expansion içinde `$name` kullanımı bu token dizisini statement veya declaration reparse aşamasına
  taşır.
- `xs_macro_expand_tokens` basit desteklenen çağrılar için call span ve genişletilmiş token listesi üretir. Bir macro
  çağrısında birden fazla desteklenen rule eşleşirse, her rule declaration order ile ayrı expansion kaydı üretir. Bu çıktı
  henüz structural AST'ye geri yazılmaz; sonraki aşamadaki fragment reparse ve AST replacement için ara genişletme akışıdır.
- `xs_macro_reparse_expansion_as_statement` desteklenen expansion token listesini synthetic bir fonksiyon gövdesi içinde
  statement olarak yeniden structural AST parser'dan geçirir. Bu köprü gerçek macro call replacement değildir; fragment-level
  reparse ve AST replacement için doğrulanabilir ara adımdır.
- `xs_macro_reparse_result_statement` reparsed synthetic gövdeden replacement statement düğümünü çıkarır. Parent-child AST
  replacement hâlâ sonraki adımdır.
- `xs_macro_expand_statements`, desteklenen token expansion'ları statement olarak yeniden ayrıştırır ve çağrı span'ı ile
  replacement statement düğümünü `XsMacroStatementExpansionSet` içinde eşler. Set, synthetic reparse ağaçlarının ownership'ini
  tuttuğu için replacement düğümleri set serbest bırakılana kadar geçerlidir. Aynı statement macro call için birden fazla
  rule eşleşirse set aynı call span’iyle declaration order içinde birden fazla replacement statement kaydı taşır. Bu API
  yalnız statement context'teki macro call'ları statement replacement olarak üretir; başka expression içinde bulunan nested
  macro call'lar token expansion düzeyinde kalır.
- `xs_macro_statement_expansion_find`, bir `XS_SYNTAX_STMT_MACRO_CALL` düğümü için synthetic replacement statement düğümünü
  macro katmanından döndürür. HIR tüketicileri replacement lookup için kendi span eşleme kodunu tutmaz.
- `xs_macro_expand_declarations`, declaration context’teki `XS_SYNTAX_DECL_MACRO_CALL` düğümlerinin desteklenen token
  expansion’larını synthetic source file olarak yeniden ayrıştırır ve `XsMacroDeclarationExpansionSet` içinde çağrı span’i,
  reparse tree ownership’i ve üretilen declaration sayısını tutar. `xs_macro_declaration_expansion_find` declaration macro
  call düğümü için ilgili synthetic declaration expansion kaydını döndürür.
- `xs_macro_expand_top_level_declarations`, top-level declaration listesi için structural expanded view üretir. Bu view
  original declaration düğümlerini korur, `XS_SYNTAX_DECL_MACRO_CALL` düğümlerini aynı call span’ine ait synthetic declaration
  expansion kayıtlarıyla declaration order içinde materyalize eder ve parent-child AST rewrite tamamlanana kadar test edilebilir
  expanded AST köprüsü sağlar.
- `xs_macro_expand_child_declarations`, aynı expanded view mekanizmasını herhangi bir parent declaration'ın doğrudan çocukları
  için üretir. HIR tip çözümleme, `class` ve `interface` içindeki declaration macro call’ların ürettiği function member ve
  field-benzeri variable declaration’larını bu view üzerinden dolaşır.
- `xs_macro_expand_child_statements`, statement block gibi parent node'ların doğrudan statement çocukları için structural
  expanded view üretir. View original statement düğümlerini korur, `XS_SYNTAX_STMT_MACRO_CALL` düğümlerini aynı call span'ine
  ait synthetic replacement statement kayıtlarıyla statement order içinde materyalize eder. Bu API fiziksel parent-child AST
  rewrite yapmaz; HIR/MIR geçişlerinin statement macro replacement'ı ortak biçimde tüketmesi için ara köprüdür.
- `xs_macro_materialize_expanded_tree`, declaration ve statement expansion set'lerini kullanarak ayrı bir syntax tree içinde
  macro-call declaration/statement düğümlerini replacement düğümleriyle klonlar. Bu ağaç orijinal AST'yi değiştirmez; reparse
  kaynak metinleri expansion set'leri tarafından sahiplenildiği için materyalize ağacın replacement text/span referansları
  expansion set lifetime'ına bağlıdır.
- HIR sembol toplama, top-level expanded declaration view üzerinden çalışır. Böylece aynı declaration macro call için birden
  fazla eşleşen rule’dan gelen tüm declaration expansion kayıtları declaration order ile normal declaration akışına girer.
- `xs check` akışı makro doğrulamadan sonra makro genişletme hazırlığını ve statement expansion set üretimini HIR sembol
  toplama aşamasından önce çalıştırır. Driver bu replacement set'in lifetime'ını compilation unit boyunca tutar ve HIR ad
  kullanımı ile HIR tip çözümleme traversal'larına verir. HIR ad ve tip çözümleme, statement child listelerini expanded
  statement view üzerinden dolaştığı için aynı statement macro call’a ait tüm replacement statement kayıtları declaration
  order ile normal statement akışına girer.

Declaration/item context macro call AST girişi, declaration reparse set üretimi, top-level/child declaration expanded view,
statement expanded view, ayrı expanded tree materyalizasyonu, HIR sembol toplama entegrasyonu ve class/interface
function/field-benzeri member expansion’ın HIR tip traversal’a bağlanması vardır. Üretilmiş declaration/statement
düğümlerinin ana AST üzerinde in-place parent-child replacement olarak yazılması ve field-benzeri declaration’ın
`XS_SYNTAX_CLASS_FIELD` olarak yeniden sınıflandırılması sonraki adımdır.

`meta` fragment yakalama ile tam AST genişletme hâlâ tamamlanmamıştır. `expr`, `stmt`, `block`, `ty`, `path`, `item` ve
`pat` fragment desteği şimdilik tek token dizisiyle sınırlıdır. Desteklenmeyen fragment matcher’lar için semantik uydurulmaz.

### MIR modeli

- `xs/mir.h` altında modül, fonksiyon bildirimi, fonksiyon tanımı, basic block ve terminator için C23 API vardır.
- MIR fonksiyon tanımları basic block listesi taşır.
- MIR fonksiyon tanımları local table taşır; local kind, tip, mutability ve isim bilgisi saklanır.
- MIR place modeli root local ve `field`/`deref`/`index` projection zinciriyle başlatılmıştır.
- MIR SSA value table ve `const.i64`, `add.i64`, `load`, `store` instruction çekirdeği vardır.
- Her basic block şimdilik `return`, `goto`, `branch` veya `unreachable` terminator’ü alabilir.
- MIR text writer declaration ve gövdeli function çıktısını deterministik yazar.
- `xs/mir/borrow_checker.h` altında ilk MIR doğrulama/borrow-check iskeleti vardır.
- Borrow checker iskeleti terminator zorunluluğunu, return type uyumunu ve immutable local köküne yapılan `store`
  işlemlerini doğrular.
- Borrow checker ayrıca instruction result/value id, `load`/`store` place id, `goto`/`branch` hedefi, `branch` condition tipi
  ve `add.i64` operand tip/id tutarlılığını doğrular.
- `xs/mir/optimizer.h` altında ilk MIR optimizasyon API’si vardır.
- CFG cleanup pass’i entry block’tan erişilemeyen block’ları kaldırır ve kalan block id / `goto` / `branch` hedeflerini
  yeniden yazar.
- Constant folding pass’i iki `const.i64` operandlı `add.i64` instruction’ını `const.i64` sonucuna indirger.

Bu aşama henüz statement/expression lowering, genel instruction set, exception edge, async state machine, region/loan/move
analizi, drop noktası doğrulaması veya kapsamlı MIR optimizasyon pass setini üretmez.

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
- `.xlil` her zaman text registry formatıdır; binary XLIL formatı eklenmeyecektir.
- XLIL CLR kadar high-level değildir, assembly kadar low-level değildir; assembly’ye benzeyen ama aynı olmayan hedef bağımsız
  orta-düşük seviye registry dilidir.
- XLIL bytecode veya sanal makine formatı değildir.
- Kararlı XLIL registry/generation C23 API hedefi `#include <xs/lil.h>` olarak belgelenmiştir.
- XLIL AOT C23 API hedefi `#include <xs/lil/aot.h>` olarak ayrılmıştır; concrete object/link davranışı gelene kadar yalnız
  planlanan public yüzeyi işaretler.
- Harici frontend ve araçlar ileride XLIL üreterek XS LLVM backend’inden, daha sonra XS Backend’inden native executable
  alabilmelidir.
- Üçüncü parti diller `xs/lil.h` ile XLIL üretebilir; XLIL AOT için `xs/lil/aot.h`, HIR baseline JIT ve MIR performance JIT
  için ayrı public başlıklar `xs/hir/jit.h` ve `xs/mir/jit.h` olarak planlanır.
- Planlanan doğrudan XLIL derleme girişi `xs build --xlil -file <girdi.xlil>` biçimindedir; komut henüz uygulanmamıştır.
- `xs/lil.h` altında hedef bağımsız XLIL modül, primitive tip, fonksiyon bildirimi, fonksiyon gövdesi, basic block,
  `const.i64` ve `return` API çekirdeği vardır.
- XLIL text writer modül başlığı, fonksiyon bildirimi ve ilk gövdeli function/block kayıtlarını yazar.
- MIR → XLIL lowering ilk gövde köprüsü `const.i64` instruction ve `return` terminator içeren düz blokları XLIL function
  body kayıtlarına indirir.
- `xs/mono/plan.h` altında ilk monomorfizasyon plan API’si vardır; şimdilik yalnız zaten concrete MIR fonksiyonlarını stable
  `_XS_FN_..._G0` sembol adına bağlar, reachable generic instantiation üretimi sonraki adımdır.
- `xs/codegen/units.h` altında hedef bağımsız codegen unit planlama API’si vardır; MIR fonksiyonları varsayılan v0
  politikasına göre module path bazlı codegen unit’lere ayrılır. Mono plandan üretildiğinde unit adı kaynak module path’inden,
  function adı stable monomorfize sembolden alınır.

XLIL instruction set, function body modeli, runtime/ABI yerleşimi ve MIR → XLIL function body lowering kararları
[TODO.md](TODO.md) altında X# v0 sözleşmesi olarak sabitlenmiştir; implementation aşamalı tamamlanacaktır.

## Henüz tamamlanmayan aşamalar

X# v0 için büyük dil, runtime, ABI, MIR, XLIL, backend ve tooling kararları [TODO.md](TODO.md) altında sabitlenmiştir.
Kalan işler karar beklemez; bu sözleşmeye göre aşamalı implementation gerektirir. Küçük uygulama kararları belgelenmiş
sözdizimi ve mevcut mimariyle uyumlu kaldığı sürece uygulama sırasında alınır.

- Makro fragment matcher eşleme motoru ve AST makro genişletmesi
- HIR metot ve operatör çözümleme
- Modül/import dışındaki tip, fonksiyon çağrısı, generic ve trait/interface bağımlılık kenarları
- Expression tip denetimi ve generic constraint doğrulaması
- Send, Sync, mutability ve async/await doğrulamaları
- MIR statement/expression lowering, exception yolları ve async state machine üretimi
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
