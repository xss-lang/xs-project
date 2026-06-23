# LLVM backend altyapısının durumu

LLVM backend, frontend’den ayrı `xs_backend_llvm` kütüphanesidir. Backend doğrudan AST veya HIR kabul etmez.
Gerçek fonksiyon gövdesi üretimi, tip denetiminden ve borrow checker’dan geçmiş monomorfize MIR düğümleri hazır
olmadan eklenmeyecektir.

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

`bool` ve `str` henüz LLVM storage tipine indirilmez:

- `bool` için `nil` temsilinin typed MIR/ABI tarafından kesinleştirilmesi gerekir.
- `str` UTF-16’dır; fakat sahiplik, uzunluk ve runtime değer yerleşimi belgelenmemiştir.

`char`, belgelenmiş 16 bit genişliği nedeniyle LLVM `i16` tipine indirilir.

Bu tipler için backend `XS_BACKEND_DEFERRED` döndürür. Böylece geçici bir ABI veya runtime yerleşimi uydurulmaz.

## Korunan aşama sınırı

Backend sırası şu şekilde korunacaktır:

```text
Borrow-checked ve optimize MIR
    → monomorfizasyon
    → codegen unit ayırma
    → LLVM module ve function signature lowering
    → MIR function body lowering
    → LLVM optimizasyonu
    → object file emission
    → linker invocation
```

MIR function body lowering henüz yoktur. Backend yalnızca fonksiyon bildirimini üretir ve basic block eklemez;
backend testi bu sınırı özellikle doğrular. Böylece AST veya tamamlanmamış HIR davranışı doğrudan LLVM IR’a
çevrilmez.
