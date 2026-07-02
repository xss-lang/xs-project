# Build ve test rehberi

xs-project C23, CMake, Ninja, Clang/LLVM ve LLD üzerine kuruludur. GNU C compiler, GNU Make generator, GNU binutils
fallback’leri, GNU C dialect’leri ve `_GNU_SOURCE` desteklenmez.

## Gereken araçlar

- `cmake`
- `ninja`
- `clang`
- LLVM araçları:
  - `llvm-ar`
  - `llvm-ranlib`
  - `llvm-nm`
  - `llvm-objcopy`
  - `llvm-objdump`
  - `llvm-strip`
- `ld.lld`

GNU dışı yardımcı araçlar tercih edilir:

- `fd`
- `rg`
- `bat -p`
- `sd`
- `busybox wc`
- `tokei`
- `uutils-coreutils` araçları

## Varsayılan build

```text
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

Preset:

- generator: Ninja
- compiler: Clang
- build dir: `build/clang-debug`
- varsayılan proje: `xs`

## OOM güvenli çalışma

Geçmişte parser/project testleri OOM üretmiştir. Uzun test/build koşularında 2GB sanal bellek limiti kullan:

```text
ulimit -v 2097152
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

Testlerin hızlı olması beklenir. Bir test aniden çok bellek tüketirse infinite loop, parser progress bug’ı veya büyüyen
macro expansion şüphesiyle ele alınmalıdır.

## Proje seçimi

Kararlı projeler:

```text
cmake --preset clang-debug -DXS_ENABLE_PROJECTS=xs
cmake --preset clang-debug -DXS_ENABLE_PROJECTS=xsproj
cmake --preset clang-debug -DXS_ENABLE_PROJECTS=all
```

`xsproj` tek başına build edildiğinde LLVM package gerektirmemelidir. Future projeler (`xsfmt`, `xstidy`, `xs-analyzer`,
`xs-backend`) şimdilik bilinçli CMake hatası üretir.

## Faydalı doğrulamalar

```text
git diff --check
rg -n "\bNULL\b|#include <stdbool\.h>" xs xsproj tests include
busybox wc -l <dosya.c> <dosya.h>
```

C ve header dosyaları 500 satırı aşmamalıdır. Yeni veya dokunulan C kodunda `nullptr` ve C23 `bool` kullanılmalıdır.

## Build çıktıları

`build/` generated alandır. Test ve build sonrası kirli görünebilir; normal commit kapsamına alınmaz.
