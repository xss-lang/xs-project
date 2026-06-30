# xs-project monorepo

Bu depo X# için LLVM-project benzeri tek checkout monorepo modelini kullanır. LLVM tarafındaki
`LLVM_ENABLE_PROJECTS` / `LLVM_ENABLE_RUNTIMES` ayrımının X# karşılığı olarak `XS_ENABLE_PROJECTS` ve
`XS_ENABLE_RUNTIMES` kullanılır.

Kök checkout adı/mantığı `xs-project`tir. Üst seviye CMake project adı `xs_project`tir.

## Etkin projeler

Üst seviye CMake seçimi:

```sh
cmake --preset clang-debug -DXS_ENABLE_PROJECTS=xs
```

Varsayılan değer `xs` projesidir. `all` değeri tüm kararlı projeleri seçer.

## Kararlı proje

- `xs`: X# derleyicisi, public C23 API başlıkları, CLI ve mevcut test hedefleri.
- `xsproj`: public C23 `.xsproj` manifest parser/lexer/model API’si.

`xs` projesinin C kaynakları `xs/sources/` altında, `xsproj` projesinin C kaynakları `xsproj/sources/` altında yaşar.

## Runtime seçimi

Üst seviye runtime seçimi:

```sh
cmake --preset clang-debug -DXS_ENABLE_RUNTIMES=
```

Şu an build edilebilir runtime yoktur. Gelecekte X# runtime ayrı bir monorepo runtime projesi olarak eklenecektir.

## Gelecek projeler

- `xsfmt`: future Rust nightly + Serde formatter project.
- `xstidy`: future Rust nightly + Serde linter project.
- `xs-analyzer`: future TypeScript VS Code extension project.
- `xs-backend`: future native XS Backend project.
- `xsrt`

Future project dizinleri kendi köklerinde tutulur ama henüz CMake build’e alınmaz. Implementation başladığında
`XS_ENABLE_PROJECTS` ile build’e alınabilir hale getirilecektir. `xsrt` runtime tarafında aynı registry mantığıyla
`XS_ENABLE_RUNTIMES` üzerinden etkinleşecektir.

## CMake sözleşmesi

- `XS_ENABLE_PROJECTS=xs`: mevcut derleyiciyi build eder.
- `XS_ENABLE_PROJECTS=xsproj`: yalnız `.xsproj` public parser kütüphanesini ve testini build eder.
- `XS_ENABLE_PROJECTS=all`: tüm kararlı projeleri build eder.
- `XS_ENABLE_RUNTIMES=`: bugün hiçbir runtime build etmez.
- `XS_ENABLE_RUNTIMES=all`: bugün hiçbir kararlı runtime olmadığı için boş listeye genişler.
- Future project adı verilirse CMake bilinçli şekilde hata verir; bu, henüz implementation olmayan araçların sessizce build’e
  girdi sanılmasını engeller.
- Future runtime adı verilirse CMake aynı şekilde bilinçli hata verir.
