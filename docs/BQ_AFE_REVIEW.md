# BQ_AFE Sürücüsü: Mevcut Durum, Bulgular ve Devam Planı

Kapsam: `BQ_AFE/` altındaki BQ79718-Q1 UART daisy-chain sürücüsü (FrameBuilder, bq79718 chip katmanı, BQ_AFE core). Amaç: bu sürücüyü BMS'e bağlanabilir hale getirirken mevcut katmanlamayı ve stili ([CODE_STYLE.md](CODE_STYLE.md)) korumak.

## 1. Mevcut Yapı (iyi olan kısımlar)

| Katman | Dosya | Durum |
|---|---|---|
| Frame | `BQ_AFE_FrameBuilder.h` | Saf, durumsuz, CRC16 tablolu. Build/validate/decapsulate ayrı. Temiz. |
| Chip | `bq79718.c/.h` + `BQ79718/` | Request builder'lar, parser, config encoder'lar ayrı. Register bit alanları union ile. Temiz. |
| Core | `BQ_AFE.c/.h` | Tek handle, ioFnMap ile UART soyutlanmış, RX burst'ü frame frame yürüyen parser. Çekirdek fikir doğru. |

Korunacak ilkeler: üç katman ayrımı, `ioFnMap` ile donanım soyutlama, builder/parser'ın durumsuz olması.

## 2. Bulgular

Önem: **K** kritik (yanlış davranış), **O** orta (BMS entegrasyonunu engeller), **D** düşük (temizlik).

| # | Önem | Yer | Bulgu |
|---|---|---|---|
| 1 | K | `BQ79718_BuildReqGpioVoltages` | `size = GPIO1_HI - GPIO11_LO` = 0x5A8 - 0x5BD, negatif; `uint8_t`'a 0xEB (235) olarak düşer, 128 sınırını aştığı için `BuildRequest` her zaman `false` döner. GPIO (termistör) okuması hiç çalışmıyor. Doğrusu `GPIO11_LO - GPIO1_HI + 1` = 22 byte. |
| 2 | K | `BQ79718_BuildReqCellVoltages` vs `BQ79718_Parse` | İstek sabit `0x0568`'den başlıyor, parser ise `BQ79718_ADDR_VCELL18_HI` = `0x574` bekliyor. Cevap frame'i istek adresini taşır, bu yüzden hücre cevabı `default` dalına düşer ve parse edilmez. Datasheet'ten hangisi doğruysa diğeri düzeltilmeli; sabit yerine makro kullanılmalı. |
| 3 | K | `BQ_AFE_HandleRxCpltIrq` + `_parseData` | IRQ, `dataReceived` diktikten hemen sonra aynı `rxBuffer`'a yeni receive-to-idle kuruyor. Ana döngü parse ederken yeni cevap buffer'ı ezebilir. Çift tampon (ping-pong) veya parse bitince yeniden kurma gerekir. |
| 4 | K | `BQ_AFE_Handle_t.data[1]`, `_parseData` | `frame.devId` sınır kontrolsüz index. Stack'te 2+ cihaz olunca dizi taşar. `data[BQ_AFE_MAX_DEVICES]` + sınır kontrolü. |
| 5 | O | `_txSendNow`, tüm `Req*/Set*` | `txBusy` sadece `BQ_AFE_Update` dönüşünde görünüyor; istek fonksiyonları busy iken de DMA'ya `txFrameG`'yi yeniden yazıyor. Busy guard veya küçük TX kuyruğu şart. |
| 6 | O | `BQ_AFE.c` | `#include "stm32g4xx.h"` ve `HAL_Delay` (wake, auto-addressing). ioFnMap'te `getTickMs` zaten var; init non-blocking FSM olmalı. Bu, "superloop non-polling" şartının sürücü tarafı. |
| 7 | O | `BQ79718_Parse`, `BQ_AFE_Data_t` | "Yeni veri geldi" bilgisi dışarı çıkmıyor: alanlar yerinde ezilir, `Parse` dönüşü yok sayılır. BMS'in haberdar olması için parse sonrası **olay callback'i** (`eventFnMap.onDataUpdated`) gerekir. Bölüm 3. |
| 8 | O | `_parseCellVoltages` | Blok VCELL18'den başlar, `cellVoltages[0]` = hücre 18. Sürücüde normalize edilmeli (index 0 = hücre 1). |
| 9 | O | `_parseDieTemp`, `_parsePackVoltages` | Ham değer; LSB dönüşümü yok (die temp ve VCELL_ACT_SUM/BAT LSB'leri hücre ADC'sinden farklı). Datasheet LSB'leri ile mV / deciDeg'e çevrilmeli. GPIO ADC LSB'sinin hücre ADC'si ile aynı olup olmadığı da doğrulanmalı (`BQ79718_ADC_RAW_TO_MV` ikisine de uygulanıyor). |
| 10 | O | Balancing | `CB_CELLx_CTRL` (0x320..0x331), `BAL_CTRL1/2/3`, `OTCB_THRESH` adres/tipte var ama builder yok. BMS setpoint'inin uygulanabilmesi için gerekli. |
| 11 | O | `BQ_AFE_Init` | `numOfDeviceCount`, `useBridge`, `deviceType` parametreleri kullanılmıyor. |
| 12 | D | `BQ79718_BuildReqDieTemp` | `sizeof(BQ79718_PackVoltagesGroup)` kullanıyor; değer (4) doğru ama tip adı yanlış. Tip `BQ79718_DieTempGroup_t` olarak yeniden adlandırılmalı. |
| 13 | D | `BQ_AFE_WakePolling` | `0x0309 <- 0x20`, `0x700 <- 0xA5`, `0x701 <- 0x20` sihirli sayılar (bridge BQ79600 register'ları gibi görünüyor). Makro + yorum. |
| 14 | D | `txFrameG` | Global, `static` değil (stil: private global `_txFrame`). |
| 15 | D | `BQ_AFE_ReqCurrent` | Stub. Register haritasında `CS_ADC_CAL1/2` ve `OC_CONF1/2` var; cihazın akım ölçümü destekleyip desteklemediği datasheet'ten doğrulanmalı. Destekliyorsa BMS için ikincil akım kaynağı olur. |

## 3. Hedef Mimari (sürücü tarafı)

### 3.1 Olay haritası: `ioFnMap`'in aynası

Sürücü veriyi kendi `BQ_AFE_Data_t` tamponunda tutmaya devam eder. BMS'e **kopya gitmez**; BMS init'te bu tamponları Attach eder, sürücü her parse sonrası sadece haber verir.

```c
typedef enum {
	BQ_AFE_DATA_GROUP_CELL_VOLTAGES = 0,
	BQ_AFE_DATA_GROUP_GPIO_VOLTAGES,
	BQ_AFE_DATA_GROUP_DIE_TEMP,
	BQ_AFE_DATA_GROUP_PACK_VOLTAGES,
	BQ_AFE_DATA_GROUP_FAULT_SUMMARY,
	BQ_AFE_DATA_GROUP_DEV_STATUS,
	BQ_AFE_DATA_GROUP_ID,
	BQ_AFE_DATA_GROUP_COUNT
}BQ_AFE_DataGroup_t;

typedef struct {
	void	(*onDataUpdated)	(uint8_t deviceIndex, BQ_AFE_DataGroup_t group, const BQ_AFE_Data_t *data, uint32_t tickMs);
	void	(*onCommError)		(uint8_t deviceIndex, uint32_t tickMs);
	void	(*onReady)			(bool ok);     /* init FSM tamamlandi */
}BQ_AFE_EventFnMap_t;

bool BQ_AFE_Init(uint8_t numOfDeviceCount, bool useBridge, BQ_AFE_DeviceType_t deviceType,
                 const BQ79718_IOFnMap_t *ioFnMap, const BQ_AFE_EventFnMap_t *eventFnMap);
```

`_parseData` içinde: `BQ79718_Parse` artık hangi grubu çözdüğünü döner (`BQ_AFE_DataGroup_t` out parametresi); başarılıysa `_hBqAfe.eventFnMap.onDataUpdated(devIdx, group, &_hBqAfe.data[devIdx], tick)` çağrılır. Parse ana döngü bağlamında (`BQ_AFE_Update`) çalıştığı için callback de ana döngüdedir; BMS tarafı O(1) işaretleme yapar.

### 3.2 Veri modeli

```c
typedef struct {
	BQ_AFE_IdGroup_t		id;
	BQ_AFE_FaultSummary_t	faultSummary;
	int16_t					cellVoltagesMv[18];     /* [0] = hucre 1 (normalize) */
	int16_t					gpioVoltagesMv[11];     /* ratiometric mV            */
	int16_t					dieTempDeciDeg[2];      /* 0.1 C (LSB donusumu)      */
	int32_t					vcellSumMv;
	int32_t					batVoltageMv;
	BQ_AFE_DevStatus_t		devStatus;
	uint16_t				commErrorCount;
	bool					online;
}BQ_AFE_Data_t;
```

### 3.3 Çalışma modeli

```
BQ_AFE_Update()  <- superloop'tan her tur (mevcut ad korunur)
  ├── RX: dataReceived ise diger tamponu parse et, her grup icin onDataUpdated
  ├── INIT FSM : WAKE -> BAUD -> BRIDGE_CFG -> AUTO_ADDR -> CONFIG -> READY
  │              (her adim tx baslat, bekleme getTickMs ile deadline, bloklama yok)
  ├── MEAS FSM : ADC_GO -> WAIT -> REQ_CELLS -> REQ_GPIO -> REQ_FAULT -> REQ_DIETEMP
  │              -> REQ_PACKV -> IDLE   (periyot konfigure edilebilir, stack read)
  └── TX kuyrugu: busy degilse siradaki frame'i gonder
```

### 3.4 Kontrol API (BMS setpoint'ini uygulamak için)

```c
BQ_AFE_Status_t BQ_AFE_SetBalanceMask		(uint8_t deviceIndex, uint32_t cellMask, uint8_t timerCode);
BQ_AFE_Status_t BQ_AFE_ReqMeasurementCycle	(void);
BQ_AFE_Status_t BQ_AFE_ResetFaults			(BQ_AFE_Target_t target);   /* mevcut */
```
BQ79718 balancing hücre başına süre sayacıyla çalışır (`CB_CELLx_CTRL`). Bağlayıcı BMS mask'ini alır, maskteki hücrelere sabit bir süre kodu yazar ve mask her değiştiğinde yeniler. Süre bitince çip kendi kapatır; bu, BMS'deki duty-cycle ile uyumlu bir "dead-man" davranışı sağlar.

## 4. BMS Bağlayıcısı (`examples/stm32_bq79718_stack/BQ_AFE_Connector.c`)

Her cihaz için init'te bir `BMS_MonitorConnector_t` kurulur (`BMS_AttachMonitorConnector`): pointer'lar sürücünün `BQ_AFE_Data_t` tamponuna, hook'lar bağlayıcının fonksiyonlarına.

| BQ_AFE verisi | Connector alanı | Haber |
|---|---|---|
| `cellVoltagesMv` (cihaz i) | `.cellVoltagesMv`, `.numOfCells` | `BMS_NotifyMonitor(…, BMS_MONITOR_DATA_CELLS, tick, flags)` |
| `gpioVoltagesMv[k]` | `.tempGroups[0]` (ntc, tsRefMv; BMS deciDeg'e çevirir) | `BMS_MONITOR_DATA_TEMP_GROUP(0)` |
| `dieTempDeciDeg` | `.tempGroups[1]` (ntc = NULL, rol AFE_DIE) | `BMS_MONITOR_DATA_TEMP_GROUP(1)` |
| `batVoltageMv` | `.moduleVoltageMv`; BMS hücre toplamı ile karşılaştırır | `BMS_MONITOR_DATA_MODULE_VOLT` |
| `faultSummary`, `commErrorCount`, `online` | bağlayıcının `_deviceStatus[i]`'sine çevrilir, `.deviceStatus` oraya bağlı | `BMS_MONITOR_DATA_STATUS` |

| BMS hook'u (BMS_Run içinden) | Bağlayıcı | Sürücü (Update'te, mesgulse sonraki tur) |
|---|---|---|
| `applyBalanceMask(module, mask)` | `_pendingMask[dev]` + bit | `BQ_AFE_SetBalanceMask(dev, mask, timerCode)` |
| `requestMeasurement` | `_measurementPending` | `BQ_AFE_ReqMeasurementCycle` |
| `clearDeviceFaults` | `_clearFaultsPending` | `BQ_AFE_ResetFaults(STACK)` |

Bağlayıcı, dengelemenin hangi cihazda açık olduğunu bildiği için (`_appliedMask`) hücre bildirimine `BMS_FEED_FLAG_BALANCING_ACTIVE` bayrağını verir. Bağlayıcı setpoint `seq` kontrol etmez; BMS değişiklikte hook'u çağırır.

## 5. Devam Adımları (sürücü hattı, BMS fazlarına paralel)

| Adım | İş | Bağımlılık |
|---|---|---|
| D1 | Bulgu 1, 2, 12, 14 düzelt (küçük, hemen) | yok |
| D2 | `data[BQ_AFE_MAX_DEVICES]` + sınır kontrolü; `Init` parametrelerini kullan; `online`, `commErrorCount` | yok |
| D3 | RX ping-pong tampon | yok |
| D4 | TX busy guard + 4-8 elemanlı TX kuyruğu | D3 |
| D5 | `BQ_AFE_EventFnMap_t` + `onDataUpdated`; `Parse` grup döndürür; hücre sırası normalize; LSB dönüşümleri; alan adları bölüm 3.2 | D2 |
| D6 | Init FSM (wake, bridge, auto-addr, config) non-blocking; `stm32g4xx.h` kaldır | D4 |
| D7 | MEAS FSM (periyodik stack read) `BQ_AFE_Update` içinde | D4, D5 |
| D8 | Balancing builder'ları + `SetBalanceMask`, `ReqMeasurementCycle` | D4 |
| D9 | Host testi: sahte UART (frame builder ile üretilen cevaplar) ile FSM'ler PC'de test | D5-D8 |
| D10 | Bağlayıcı + BMS ile entegrasyon | BMS Faz 1-3, D7, D8 |

D1-D5 mevcut davranışı bozmadan yapılabilir. D6 ve D7 çalışma modelini değiştirir; önce hedef kartta mevcut blocking init ile doğrulanmış bir referans ölçüm alınmalı, sonra FSM'e geçilmeli.

## 6. Doğrulanacaklar (datasheet SLUSG69)
- VCELL18_HI gerçek adresi: 0x568 mi 0x574 mü (bulgu 2).
- GPIO ADC LSB (hücre ADC ile aynı mı).
- DIETEMP LSB ve offset; VCELL_ACT_SUM ve BAT LSB.
- CS ADC var mı, hangi register'lar (bulgu 15).
- CB_CELLx_CTRL süre kodları ve BAL_CTRL2 bit anlamları (AUTO_BAL, BAL_GO, OTCB_EN).
