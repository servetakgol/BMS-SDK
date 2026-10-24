# Örnek: 32 × BQ79718 daisy-chain, IVT CAN akım sensörü, shunt ADC, harici kontaktör

SDK'nın connector modelinde entegrasyonun nasıl göründüğünü gösterir. Tek superloop, RTOS yok, hiçbir yerde bekleme döngüsü yok, hiçbir yerde tarama yok. Kod stili: `docs/CODE_STYLE.md`.

## Üç katman, üç connector

```
  SÜRÜCÜLER (BMS'i bilmez)           CONNECTOR'LAR (uygulama)             BMS
  ─────────────────────────          ──────────────────────────           ─────────────────
  BQ_AFE      ──eventFnMap──▶  BQ_AFE_Connector    ──Attach/Notify──▶  monitor connector ×32
  IVT_CAN     ──eventFnMap──▶  Current_Connector   ──Attach/Notify──▶  current connector (primary)
  SHUNT_ADC   ──eventFnMap──▶        "             ──Attach/Notify──▶  current connector (secondary)
  Board GPIO  ◀──────────────  Contactor_Connector ◀──hook: applyCommand──  kontaktör FSM
  BQ_AFE      ◀──Update──────  BQ_AFE_Connector    ◀──hook: applyBalanceMask──  balancing
```

Sürücüler (`BQ_AFE`, `IVT_CAN`, `SHUNT_ADC`) aynı deseni paylaşır: veri kendi `Data_t` tamponunda, güncellenince `eventFnMap.onDataUpdated`. Hangi sürücünün hangi BMS kaynağına bağlanacağı **uygulamanın** kararıdır ve connector dosyalarında yaşar. Akım IVT'den mi shunt'tan mı geliyor, BMS bilmez.

## Dosyalar

| Dosya | Rol |
|---|---|
| `BMS_UserConfig.h` | SDK derleme zamanı üst sınırları |
| `App_Config.h/.c` | Topoloji sabitleri, `appSystemCfgG`: pack konfigürasyonu, fault tablosu, limitler, balancing |
| `BQ_AFE_Connector.h/.c` | BQ_AFE ↔ `BMS_MonitorConnector_t` ×32. Init'te Attach, callback'te `BMS_NotifyMonitor`, hook'lar istekleri biriktirir, `Update` sürücüye aktarır |
| `IVT_CAN.h/.c` | IVT CAN sürücüsü: frame → `IVT_CAN_Data_t`, `onDataUpdated(dataMask)` |
| `SHUNT_ADC.h/.c` | Shunt ADC sürücüsü: DMA → ortalama → `SHUNT_ADC_Data_t`, `onDataUpdated` |
| `Current_Connector.h/.c` | IVT = birincil, shunt = ikincil kararı. İki `BMS_CurrentConnector_t` Attach, callback'lerde `BMS_NotifyCurrent` |
| `Contactor_Connector.h/.c` | `BMS_ContactorConnector_t`: `applyCommand` hook'u → GPIO; feedback her tur okunur, değişince `BMS_NotifyContactor` |
| `Board.h`, `Board_STM32G4.c` | HAL'ı gören tek dosya. ISR'lar burada biter. `BMS_PortFnMap_t` hedefleri |
| `main.c` | Fn map'lerin kurulması, superloop, telemetri, NVM, BMS olayları |

## Superloop sırası (main.c)

1. `BQ_AFE_Update()` sürücü FSM'leri; parse ettiği her grup için `onDataUpdated` → `BQ_AFE_CONN_OnDataUpdated` → `BMS_NotifyMonitor` (O(1)).
2. `_drainCanRx` → `IVT_CAN_OnFrame` → `onDataUpdated` → `CURRENT_CONN_OnIvtDataUpdated` → `BMS_NotifyCurrent`.
3. `SHUNT_ADC_Update` → `onDataUpdated` → `CURRENT_CONN_OnShuntDataUpdated` → `BMS_NotifyCurrent`.
4. `CONTACTOR_CONN_Update` feedback GPIO'larını okur, değişince `BMS_NotifyContactor`.
5. `BMS_Run()` tek tur. Setpoint değişirse **BMS** hook'ları çağırır: `applyBalanceMask` (değişen modüller), `applyCommand`.
6. `BQ_AFE_CONN_Update` hook'ların biriktirdiği mask'leri sürücüye yazar; sürücü meşgulse sonraki tur.
7. Telemetri ve watchdog.

## Kopyalama sorusunun cevabı

576 hücre, 1152 byte voltaj. Kopyalamak CPU'da ~2 µs; sorun RAM ve kod tekrarı. Connector modelinde BMS hiçbir diziyi kopyalamaz: pointer'lar init'te bağlanır, Notify sadece dirty bit + tick yazar, `BMS_Run()` dirty olanı yerinde okur.

| | BMS ek RAM (32 cihaz) | Her yeni kaynak için |
|---|---|---|
| Kopya modeli | ~5.7 KB | kopya kodu + staging |
| Connector (Attach + Notify) | ~2.2 KB | init'te 1 Attach, callback'te 1 Notify |

Bağlama sözleşmesi: pointer init'ten sonra değişmez; tampon `BMS_Run()` ile aynı bağlamda (ana döngü) yazılır; yazıcı her tutarlı güncellemeden sonra Notify çağırır. Hook'lar `BMS_Run()` içinden çağrılır, bloklamaz; sürücü meşgulse connector bekletir.

## Hedef sürücü API'si

`BQ_AFE_Connector`, sürücünün **hedef** yüzeyini kullanır (`docs/BQ_AFE_REVIEW.md` bölüm 3 ve 5). Bugünkü sürücüde henüz olmayanlar:

- `BQ_AFE_EventFnMap_t` ve `BQ_AFE_Init`'e beşinci parametre; `BQ_AFE_DataGroup_t`; parse sonrası `onDataUpdated` — adım D5
- `BQ_AFE_Data_t` alan adları ve birimleri (`cellVoltagesMv[0] = hücre 1`, `dieTempDeciDeg`, `batVoltageMv`), `online`, `commErrorCount` — adım D2, D5
- `BQ_AFE_MAX_DEVICES` boyutlu veri dizisi — adım D2
- `BQ_AFE_Update()` içinde non-blocking init + ölçüm FSM — adım D6, D7
- `BQ_AFE_SetBalanceMask`, `BQ_AFE_ReqMeasurementCycle` — adım D8

## Sözdizimi kontrolü

Örnek henüz linklenmez (SDK implementasyonu Faz 1-5). Başlıklar ve örnek, HAL ve hedef BQ_AFE API'si için stub başlıklarla doğrulanır:

```bash
arm-none-eabi-gcc -fsyntax-only -std=c11 -Wall -Wextra -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -I include -I examples/stm32_bq79718_stack -I <stub-dir> examples/stm32_bq79718_stack/*.c
```
