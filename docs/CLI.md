# CLI sözleşmesi

`xs` komutu bugün proje manifesti üzerinden çalışır. Manifest syntax’ı `.xsproj` formatıdır; JSON/TOML/YAML değildir.

## Desteklenen biçimler

```text
xs check -proj MyApp.xsproj
xs build -proj MyApp.xsproj
xs run -proj MyApp.xsproj
xs build --output hir -proj MyApp.xsproj
xs build --output mir -proj MyApp.xsproj
xs build --output xlil -proj MyApp.xsproj
```

Kullanım hatasında CLI şu biçimi gösterir:

```text
kullanım: xs <check|run> -proj <proje.xsproj>
kullanım: xs build [--output hir|mir|xlil] -proj <proje.xsproj>
```

## `xs check`

`xs check` object üretmez. Bugünkü akış:

1. `.xsproj` parse/model validation
2. source discovery
3. X# parse/structural AST
4. macro validation ve expansion preparation
5. HIR symbol/import/name/type resolution
6. erken expression checks

## `xs build`

`xs build` gelecekte check, MIR, borrow checker, monomorphization, XLIL, backend, object ve link akışını çalıştıracaktır.
Bugün bazı output modları tamamlanmamış ara formatlar nedeniyle bilinçli failure üretebilir.

## `xs run`

`xs run` gelecekte önce `xs build` yapıp üretilen executable’ı çalıştıracaktır. Native executable akışı tamamlanana kadar
tam run semantiği hazır kabul edilmez.

## Ara çıktılar

- `.xhir`: HIR text
- `.xmir`: MIR text
- `.xlil`: XLIL text registry

`.xlil` hiçbir zaman binary format olmayacaktır.

## Future direct XLIL build

Planlanan biçim:

```text
xs build --xlil -file foo.xlil
```

Bu yol proje manifesti okumadan XLIL parse/verify/backend/link akışına gidecektir. Henüz uygulanmamıştır.
