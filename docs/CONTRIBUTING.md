# Katkı ve çalışma kuralları

Bu proje compiler geliştirme deposudur. Küçük, test edilebilir ve mimari sınırları koruyan patch’ler tercih edilir.

## Temel kurallar

- C23 ana implementation dilidir.
- Rust yalnız future tool projelerinde (`xsfmt`, `xstidy`) veya açık teknik gerekçeyle izole modüllerde kullanılmalıdır.
- Shell script kalıcı proje artifact’i olarak eklenmez.
- CMake kullanılır; Meson kullanılmaz.
- GNU toolchain davranışlarına bağımlılık eklenmez.
- C ve header dosyaları 500 satır sınırını aşmamalıdır.
- Yeni/touched C kodunda `NULL` yerine `nullptr`, `<stdbool.h>` yerine C23 `bool` kullanılır.

## Patch boyutu

Bir compiler aşamasını tek dev patch ile tamamlamaya çalışma. Tercih edilen biçim:

1. veri modelini ekle
2. parser/HIR/MIR bağlantısını kur
3. focused test ekle
4. dokümantasyonu güncelle
5. build/test geçir

## Test

Her değişiklik en azından touched subsystem testini geçirmelidir. Pratik olduğunda tam test seti çalıştırılır:

```text
ulimit -v 2097152
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

Docs-only değişikliklerde build zorunlu değildir; fakat `git diff --check` çalıştırılmalıdır.

## Dokümantasyon

Kod davranışı değişirse ilgili belge de değişir. Kullanıcıya görünür her değişiklik kök `CHANGELOG.md` altında özetlenir.

## Commit

Generated artifact’leri commit’e alma:

- `build/`
- `target/`
- `node_modules/`
- `dist/`
- `out/`

Kullanıcının değişikliklerini koru; ilgisiz dosyaları geri alma.
