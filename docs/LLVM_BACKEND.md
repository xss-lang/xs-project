# LLVM backend altyapısının durumu

LLVM backend, frontend’den ayrı `xs_backend_llvm` kütüphanesidir. Backend doğrudan AST veya HIR kabul etmez.
HIR ve MIR LLVM’e bağlı değildir; fakat hedef bağımsız XLIL tip/veri sözlüğüne bağlıdır. LLVM C API, target triple ve
data layout kavramları yalnızca backend katmanında bulunur. Gerçek fonksiyon gövdesi üretimi, tip denetiminden ve
borrow checker’dan geçmiş monomorfize MIR düğümleri XLIL’e indirildikten sonra eklenecektir.

LLVM şu an ana backend odağıdır, ancak HIR/MIR/XLIL tasarımı başka backend’lere taşınabilir kalmalıdır. Hedefe özel
assembly gerekirse backend/runtime katmanında izole tutulur; NASM `.asm`/`.inc` kullanımı x86-64’e kilitlenmeden,
ARM64 uyumluluğu korunarak yapılabilir.

## Uygulanan bileşenler

- Bir LLVM context ve target machine yaşam döngüsü
- Açık target triple veya yerel target triple seçimi
- Target data layout üretimi
- Her codegen unit için bağımsız LLVM module
- Sayısal X# primitive tiplerinin LLVM tiplerine eşlenmesi
- Gövdesiz fonksiyon bildirimi ve signature lowering
- `default<O0>` ile `default<O3>` arasında LLVM optimizasyon pipeline seçimi
- LLVM module doğrulaması
- Codegen unit başına object file emission
- Shell kullanmadan, argümanları çağıran tarafından verilen linker invocation katmanı

Nesne dosyası üretimi ve linker çağrısı altyapı olarak çalışır; henüz `xs build` komutuna bağlanmamıştır.

## Bilerek ertelenen tip eşlemeleri

`str` henüz LLVM storage tipine indirilmez:

- `str` UTF-16’dır.
- `str` uzunluğu UTF-16 temsilinin izin verdiği ölçüde sınırsızdır.
- Sahiplik, uzunluk alanı türü ve runtime değer yerleşimi henüz belgelenmemiştir.

`bool`, HIR aşamasında 1 bit primitive olarak çözülür ve LLVM backend tarafından `i1` tipine indirilir.

`byte` ve `sbyte` HIR düzeyinde ayrı unsigned/signed 8 bit primitive tiplerdir. LLVM storage tipi ikisi için de
`i8` olur; signedness kararı karşılaştırma, dönüşüm ve aritmetik operasyon seçimi sırasında typed MIR’dan gelir.

`char`, belgelenmiş 16 bit genişliği nedeniyle LLVM `i16` tipine indirilir.

`str` için backend `XS_BACKEND_DEFERRED` döndürür. Böylece geçici bir ABI veya runtime yerleşimi uydurulmaz.

## Korunan aşama sınırı

Backend sırası şu şekilde korunacaktır:

```text
Borrow-checked ve optimize MIR
    → monomorfizasyon
    → codegen unit ayırma
    → XLIL
    → LLVM module ve function signature lowering
    → XLIL function body lowering
    → LLVM optimizasyonu
    → object file emission
    → linker invocation
```

XLIL function body lowering henüz yoktur. Backend yalnızca fonksiyon bildirimini üretir ve basic block eklemez;
backend testi bu sınırı özellikle doğrular. Böylece AST veya tamamlanmamış HIR davranışı doğrudan LLVM IR’a
çevrilmez.
