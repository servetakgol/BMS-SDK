# Kod Stili

Kaynak: `BQ_AFE/` sürücüsü. SDK, bağlayıcılar (connector) ve örnekler aynı stili kullanır.

## Dosyalar

- Modül dosyaları: `BMS.c/.h`, `BMS_Types.h`, `BMS_Cfg.h`, `BMS_Feed.h`, `BMS_Out.h` (modül adı büyük, alt ad PascalCase, `_` ile).
- Çip/alt bileşen dosyaları küçük harf olabilir: `bq79718.c`, `bq79718_RegAddr.h`.
- Her dosya şu başlıkla açılır:

```c
/*
 * BMS_Feed.h
 *
 *  Created on: Oct 14, 2026
 *      Author: servet
 */
```

- Header guard: dosya adı büyük harf + `_H_`: `#ifndef BMS_FEED_H_` … `#endif /* BMS_FEED_H_ */`.
- Include sırası: `<stdbool.h>`, `<stdint.h>`, `<stddef.h>`, `<string.h>`, boş satır, yerel başlıklar.

## İsimlendirme

| Öğe | Kural | Örnek |
|---|---|---|
| Public fonksiyon | `MODUL_PascalCase` | `BQ_AFE_Init`, `BMS_Run`, `BMS_FeedCurrent` |
| Alt modül fonksiyonu | `MODUL_ALTMODUL_PascalCase` | `BQ_AFE_FRAME_BuildWrite`, `BMS_FAULT_Eval` |
| Private (static) fonksiyon | `_camelCase` | `_txSendNow`, `_parseData` |
| Private (static) global | `_camelCase`, handle için `_h<Ad>` | `_hBqAfe`, `_lastSetpointSeq` |
| Public global | camelCase + `G` soneki | `txFrameG`, `appSystemCfgG` |
| Tip | `MODUL_PascalCase_t` | `BQ_AFE_Handle_t`, `BMS_PackCfg_t` |
| Enum değeri | `MODUL_GRUP_DEGER` | `BQ_AFE_STATUS_OK`, `BMS_TEMP_ROLE_CELL` |
| Makro / sabit | `MODUL_BUYUK_HARF` | `BQ_AFE_HEADER_LEN`, `BMS_CFG_MAX_MODULES` |
| Alan ve yerel değişken | camelCase, birim soneki ile | `cellVoltagesMv`, `adcDelayUs`, `numOfDeviceCount`, `tickMs` |
| Fonksiyon işaretçisi haritası | `..FnMap_t`, alanlar camelCase | `BQ79718_IOFnMap_t { uartTransmit, getTickMs }` |
| Dönüş durumu | `MODUL_Status_t` enum | `BQ_AFE_STATUS_OK / _BUSY / _ERROR` |

Birim sonekleri: `Mv`, `Ma`, `Mas`, `Ms`, `Us`, `Ohm`, `DeciDeg` (0.1 °C), `Pct` (0.01 %). Sayı: `numOf…`, index: `…Index`.

## Biçim

- Girinti **tab**. Hizalama için de tab (struct alanları, enum atamaları).
- Fonksiyon açılış parantezi aynı satırda: `bool BQ_AFE_Init(...) {`. Gövde başında bir boş satır serbest.
- `if (...) {` ile boşluk; tek satırlık erken dönüşte `{}`siz biçim de kabul: `if (x)\n\t\treturn ...;`
- Pointer yıldızı değişkene bitişik: `const uint8_t *data`.
- `typedef struct { ... }BMS_X_t;` (kapanış ile ad arasında boşluk yok, mevcut kodla uyumlu).
- Register / bayrak kümeleri `union { raw; struct { ... } mapped; }` veya bit alanı struct.
- Bölüm başlıkları:

```c
/* ====================================================================== */
/*  Cevap dispatch (BQ79718 parser'a)                                     */
/* ====================================================================== */

/* ---- Read-request builders -------------------------------------------- */
```

- Yorumlar `/* */`; Türkçe veya İngilizce serbest, `// todo : ...` ile açık iş.
- Sihirli sayı yok: `UINT32_C(...)` veya `#define`.

## Desenler

- Donanım bağımlılığı fonksiyon işaretçisi haritasıyla içeri verilir (`ioFnMap`), dışarı olaylar da aynı şekilde (`eventFnMap`). `extern` ile beklenen port fonksiyonu yok.
- Tek statik handle (`_hX`), `Init` içinde `memset` ile sıfırlanır.
- ISR handler'ları `MODUL_Handle<Olay>Irq` adıyla, sadece bayrak/kuyruk yazar.
- Periyodik iş `MODUL_Update(void)`; bloklamaz, `getTickMs` ile zaman alır.
