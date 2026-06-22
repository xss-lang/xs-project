# X# derleyici uygulaması

Uygulama, `XS/CompilerFlow.txt` dosyasındaki sırayı takip eder:

1. Sözcüksel analiz ve ayrıştırma sonucunda AST üretimi
2. AST üzerinde makro genişletme
3. HIR oluşturma ve bağımlılık çözümleme
4. HIR üzerinde tip denetimi
5. MIR oluşturma
6. Optimize edilmemiş MIR üzerinde ödünç alma denetimi
7. MIR optimizasyonu
8. Monomorfizasyon
9. Kod üretim birimlerine ayırma ve önbellekleme
10. LLVM IR üretimi ve optimizasyonu
11. Nesne kodu üretimi ve bağlama

Mevcut aşamada derleyicinin C23 ve Clang proje iskeleti, kaynak aralıkları, tanılar, belgelenmiş tokenlar,
yorum ve metinlerin sözcüksel analizi, AST depolama yapısı ve üst düzey bildirim sınırlarının ayrıştırılması
uygulanmıştır. ASCII tanımlayıcılar; ayrılmış sözcükler; onluk, bilimsel gösterimli ve ayraçlı sayı sabitleri
doğrulanmaktadır. `.xsproj` dosyaları kendi belgelenmiş sözdizimleriyle ayrıştırılıp şema açısından denetlenmektedir.
`macroRules!` bildirimleri AST'de temsil edilmekte ve temel kural biçimleri ayrıştırılmaktadır.

Bu aşamayı yapılandırmak, derlemek ve test etmek için:

```text
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug
```

Hazır ayar Clang ve Ninja kullanır. CMake projesi GNU C derleyicisini ve Makefile üreteçlerini reddeder.

Bir örnek projeyi denetlemek için:

```text
./build/clang-debug/xs check -proj XS/example/MyApp.xsproj
```

`build` ve `run` komut biçimleri tanınmaktadır; ancak arka uç aşamaları tamamlanmadan çıktı üretilmez.

## Bilerek sonraya bırakılan işler

- Parametreleri, tipleri, genel tip kısıtlarını, bildirim üyelerini, ifadeleri ve deyimleri yapısal AST düğümlerine
  ayrıştırmak. Fonksiyon ve bildirim gövdeleri şimdilik kaynak aralığı olarak saklanmaktadır.
- Makro eşleyicilerini yapısal AST parçalarına dönüştürmek; kapsam toplama, çoklu eşleşme, tekrarlar, genişletme ve
  doğrudan/dolaylı özyineleme döngüsü denetimini AST ile HIR arasındaki makro aşamasında uygulamak.
- Proje grafiğinde modül adlarının benzersizliğini ve import çözümlemesini HIR bağımlılık grafiğiyle birlikte
  uygulamak. Belgelerde import edilecek modül dosyalarının dosya sisteminde nasıl aranacağı tanımlanmadığından bu
  arama kuralını tahmin etmemek.
- Anlamsal doğrulamayı parser içinde tahmin etmek yerine HIR, tip denetimi ve MIR aşamalarında uygulamak.
