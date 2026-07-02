# xs-project documentation

Bu dizin X# derleyicisinin aktif mimari ve implementation belgelerini tutar. `Spec/` dizini syntax ve dil örnekleri için
kaynak belge alanıdır; `docs/` ise derleyici mimarisi, build/test süreci, CLI sözleşmesi ve implementation durumunu açıklar.

## Okuma sırası

Yeni başlayan biri için önerilen sıra:

1. [../README.md](../README.md): depo ve hızlı başlangıç
2. [BUILDING.md](BUILDING.md): build/test ve toolchain
3. [ARCHITECTURE.md](ARCHITECTURE.md): compiler pipeline ve katman sınırları
4. [CLI.md](CLI.md): kullanıcı komutları ve mevcut durum
5. [IMPLEMENTATION.md](IMPLEMENTATION.md): aşama aşama implementation durumu
6. [TODO.md](TODO.md): X# v0 kararları ve kalan implementation işi
7. [LLVM_BACKEND.md](LLVM_BACKEND.md): LLVM backend altyapısı
8. [MONOREPO.md](MONOREPO.md): monorepo proje/runtime seçim modeli

## Belge otoritesi

- Belgelenmiş X# syntax için `Spec/` önce gelir.
- `.xsproj` biçimi için `Spec/ProjectSystem.xs` ve public C API başlıkları önce gelir.
- XLIL kararları için `XS/XLIL.md`, `docs/TODO.md` ve `xs/include/xs/lil.h` birlikte değerlendirilir.
- Implementation sırası için [IMPLEMENTATION.md](IMPLEMENTATION.md) otoritedir.
- Büyük semantik/ABI kararları [TODO.md](TODO.md) içinde X# v0 sözleşmesi olarak tutulur.

Çelişki görülürse sessizce yeni davranış ekleme; belgeyi ve implementation’ı aynı patch içinde uyumlu hale getir.

## Güncelleme kuralı

Kod davranışı değiştiğinde en az bir belge güncellenmelidir:

- CLI değiştiyse `CLI.md`.
- Build/toolchain değiştiyse `BUILDING.md`.
- Pipeline veya katman sınırı değiştiyse `ARCHITECTURE.md` ve `IMPLEMENTATION.md`.
- Büyük dil/ABI kararı eklendiyse `TODO.md`.
- Kullanıcıya görünür değişiklik olduysa kök `CHANGELOG.md`.
