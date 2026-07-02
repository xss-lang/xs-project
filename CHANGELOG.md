# Changelog

Bu dosya xs-project deposundaki kullanıcıya ve geliştiriciye görünür değişiklikleri özetler.

Numaralı sürümler LLVM IR üretimi çalışır hale geldikten sonra başlayacaktır. O aşamaya kadar tüm değişiklikler
`Unreleased` altında tutulur; bu başlık release sözü değil, geliştirme günlüğüdür.

## Unreleased

### Eklenenler

- LLVM-project tarzı monorepo yapısı.
- `xs` derleyici projesi ve `xsproj` public `.xsproj` manifest parser/model API’si.
- X# lexer, structural AST parser ve temel diagnostic sistemi.
- Macro validation, token expansion, statement/declaration reparse ve expanded view altyapısı.
- HIR symbol table, import/name/type resolution başlangıcı.
- Generic arity, duplicate generic parameter ve generic interface constraint doğrulamaları.
- Primitive tip metadata: `bool`, `byte`, `sbyte`, `char`, integer/float ailesi ve `str` bilgisi.
- MIR model API, MIR writer, borrow-check iskeleti ve ilk optimizer pass’leri.
- XLIL model/writer ve sınırlı MIR → XLIL body lowering.
- LLVM backend altyapısı: context/module/target/data layout, signature lowering, object emission ve linker abstraction.
- `xs check -proj ...` ve `xs build --output hir|mir|xlil -proj ...` CLI yolları.
- Kök README ve güçlendirilmiş `docs/` dokümantasyon seti.

### Değişenler

- Module selected import sözcüğü `froms` yerine `from` oldu.
- `byte` HIR düzeyinde `u8`, `sbyte` HIR düzeyinde `i8` olarak sabitlendi.
- `char` 16-bit UTF-16 code unit olarak belgelendi.
- `str` UTF-16 ve platform/runtime representation sınırına kadar uzun kabul edildi.
- HIR/MIR katmanlarının LLVM’e bağlı olmaması ve XLIL sözlüğüne bağlı kalması dokümante edildi.
- Structural parser generic type/generic parameter kapanışında `>>` tokenını iki `>` gibi tüketebilir.

### Bilinen eksikler

- Tam AST macro replacement henüz in-place parent-child rewrite yapmaz.
- Genel expression type inference, overload resolution ve nominal interface üyelik doğrulaması tamamlanmadı.
- MIR statement/expression lowering, exception edge ve async state machine üretimi tamamlanmadı.
- Borrow checker henüz tam region/loan/move/drop modeli değildir.
- `xs build`/`xs run` uçtan uca object/link/native executable akışı tamamlanmadı.
- Direct XLIL build (`xs build --xlil -file ...`) henüz uygulanmadı.
