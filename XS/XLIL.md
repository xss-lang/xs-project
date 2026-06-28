XLIL (X# Low-Level Intermediate Language)

Genel Bakış

XLIL (X# Low-Level Intermediate Language), X# derleyicisinin resmi düşük seviyeli ara dilidir (Intermediate Language).

XLIL, LLVM IR'dan önce yer alır ve tüm backend'lerin ortak giriş noktası olarak tasarlanmıştır.

Amaç

- X# için standart bir düşük seviyeli ara dil oluşturmak.
- LLVM IR'a doğrudan bağımlılığı ortadan kaldırmak.
- Tüm backend'lerin ortak bir ara dili kullanmasını sağlamak.
- Gelecekte geliştirilecek XS Backend'in temelini oluşturmak.
- Başka programlama dillerinin XLIL üreterek XS Backend'i hedefleyebilmesini sağlamak.

Özellikler

XLIL;

- Düşük seviyeli bir ara dildir.
- Backend'den bağımsızdır.
- Assembly'e benzer ancak assembly değildir.
- Hedef mimariden bağımsızdır.
- Yerel makine kodu üretimi için tasarlanmıştır.
- Bytecode veya sanal makine ara dili değildir.

Harici API

XLIL için kararlı bir C23 API'si sağlanacaktır. (#include <xs/lil.h>)

Bu API sayesinde;

- Başka programlama dilleri XLIL üretebilir.
- Üçüncü taraf derleyiciler XLIL'i hedefleyebilir.
- Alternatif frontend'ler geliştirilebilir.
- XLIL üreten tüm derleyiciler ortak backend altyapısından yararlanabilir.
