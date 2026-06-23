# X# derleyicisinin mevcut durumu

Derleyici C23 ile yazılır; Clang, CMake, Ninja, LLVM ve LLD kullanır. GNU C derleyicisi ve Makefile üreteçleri
CMake yapılandırması tarafından reddedilir.

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
    → LLVM IR ve LLVM optimizasyonu
    → nesne kodu
    → bağlama
```

## Tamamlanan altyapı

### Proje sistemi

- `.xsproj` dosyaları özgün X# proje sözdizimiyle ayrıştırılır.
- Zorunlu alanlar, tekrar eden alanlar, bilinmeyen alanlar ve `appRelease` değerleri doğrulanır.
- `entry: nil` olduğunda belgelenmiş ilk ek kaynak seçimi uygulanır.
- Proje içindeki göreli yollar `.xsproj` dosyasının bulunduğu dizinden çözülür.
- `xs check -proj <proje.xsproj>` çalışır.

### Lexer ve yapısal AST

- Belgelenmiş anahtar sözcükler, operatörler, yorumlar ve çok satırlı metinler tokenlaştırılır.
- ASCII tanımlayıcı kuralları uygulanır.
- Onluk tam sayılar, kayan noktalı sayılar, bilimsel gösterim ve `'` basamak ayırıcıları doğrulanır.
- Parser arena tabanlı yapısal AST üretir.
- AST düğümleri dosya kimliği, offset, satır ve sütun içeren tam kaynak konumu taşır.
- Bildirim, tip, statement, expression, pattern ve makro düğüm aileleri temsil edilir.
- Fonksiyon parametreleri, dönüş tipi, `throws` tipleri ve fonksiyon gövdeleri yapısal düğümlerdir; ham gövde aralığı
  olarak saklanmaz.
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

Bu katman HIR dizini altında bulunur; genel ad çözümleme HIR’ı henüz tamamlanmamıştır.

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

## Henüz tamamlanmayan aşamalar

- Makro matcher eşleme motoru, kapsam toplama ve AST makro genişletmesi
- HIR ad, namespace, görünürlük, metot ve operatör çözümleme
- Modül/import dışındaki tip, fonksiyon çağrısı, generic ve trait/interface bağımlılık kenarları
- Tip denetimi ve generic constraint doğrulaması
- Send, Sync, mutability ve async/await doğrulamaları
- MIR üretimi, exception yolları ve async state machine üretimi
- Borrow checker ve drop noktalarının doğrulanması
- MIR optimizasyonları
- Monomorfizasyon, codegen unit ayırma ve artımlı derleme önbelleği
- MIR fonksiyon gövdelerinin LLVM IR’a indirilmesi
- Runtime/ABI yerleşimi tamamlanmamış `bool` ve `str` tiplerinin LLVM eşlemesi
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
