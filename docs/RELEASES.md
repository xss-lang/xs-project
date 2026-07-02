# Sürüm politikası

xs-project henüz numaralı release üretmez. Sürüm dönemi LLVM IR üretimi çalışır hale geldikten sonra başlayacaktır.

Bu kararın nedeni pipeline sınırıdır: parser, AST, macro, HIR, MIR, borrow checker, XLIL ve LLVM backend katmanları parça parça
ilerlerken kullanıcıya “sürüm” adıyla yarı tamamlanmış native derleyici sözü verilmez.

## Bugünkü durum

- Kök [../CHANGELOG.md](../CHANGELOG.md) dosyası `Unreleased` geliştirme günlüğüdür.
- `Unreleased` başlığı kararlı release anlamına gelmez.
- Commit ve test geçmişi implementation ilerlemesini gösterir.

## İlk sürüm eşiği

İlk numaralı sürüm için beklenen minimum eşik:

1. typed ve borrow-check geçmiş MIR’den XLIL’e function body lowering,
2. XLIL’den LLVM IR function body lowering,
3. LLVM module verification,
4. object emission,
5. proje hedeflerine göre link,
6. `xs build` ile native artifact üretimi.

Bu eşik tamamlandıktan sonra changelog `Unreleased` altında biriken kullanıcıya görünür değişiklikler ilk sürüm başlığına
taşınabilir.
