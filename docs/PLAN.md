# Battery Manager SDK (BMS) — Mimari Plan

Hedef: STM32 üzerinde, RTOS'suz **superloop** içinde çalışan, **non-blocking / non-polling** (olay güdümlü), bir veya birden fazla paketi yöneten, donanımdan ve sürücüden bağımsız bir Battery Management motoru. Saf C (C99/C11), dinamik bellek yok, statik konfigürasyon.

## 0. Alınan Kararlar

| Konu | Karar |
|---|---|
| Entegrasyon modeli | **SDK saf motor, connector ile bağlanır.** Sürücüler dışarıda çalışır. Uygulama init'te her kaynak için bir **connector** kurar: veri pointer'ları (sıfır kopya) + BMS'in çağıracağı hook'lar. Veri gelince kaynak `BMS_Notify<Tip>` çağırır (O(1)). Setpoint değişince BMS hook'u çağırır. SDK hiçbir SPI/I2C/CAN portu istemez, hiçbir kaynağı taramaz. (Gerekçe: bölüm 2) |
| Üç connector tipi | `BMS_MonitorConnector_t` (AFE: hücre, sıcaklık grupları, modül V, cihaz durumu; balance/FET/ölçüm hook'ları), `BMS_CurrentConnector_t` (akım, As sayacı, pack/bus V, shunt sıcaklığı; kaynağın IVT mi shunt mı olduğu uygulamanın kararı), `BMS_ContactorConnector_t` (komut hook'u, feedback). |
| Sürücü → connector haberleşme | Her sürücüde `ioFnMap`'in aynası bir `eventFnMap` (`onDataUpdated(…, tickMs)`). Connector bu callback'te Notify çağırır. BQ_AFE, IVT_CAN, SHUNT_ADC aynı deseni kullanır. |
| Mevcut BQ sürücüsü | `BQ_AFE/` altındaki **BQ79718-Q1 UART daisy-chain** sürücüsü devam ettirilir. Katmanlaması korunur. Bulgular ve devam adımları: [BQ_AFE_REVIEW.md](BQ_AFE_REVIEW.md). |
| Kod stili | BQ_AFE stili: [CODE_STYLE.md](CODE_STYLE.md). `BMS_PascalCase` fonksiyon, `BMS_X_t` tip, camelCase alan, `_private`, `FnMap_t` desenleri, tab girinti. |
| Hedef MCU | Yüksek işlem gücü (G4/H7 sınıfı). |
| Sayısal model | **Hesap float, saklama ve iletim tam sayı.** Tüm giriş/çıkış ve durum değerleri `int32_t/uint32_t/int16_t` (mV, mA, deciDeg, 0.01 %, mAs). Fonksiyon içinde aritmetik float ile yapılır, sonuç `lroundf` ile tam sayıya yazılır. Float ara değerler struct'larda tutulmaz; tek istisna, hassasiyet kaybı tam sayıyla önlenemeyen estimator iç durumu. |
| Kimya / OCV | Şimdilik yer tutucu. `BMS_Chemistry_t` slotu ve estimator arayüzü planlanır, içi sonra doldurulur. |
| Dış arayüz (CAN/UART telemetri) | SDK kapsamı dışı. SDK snapshot verir, arayüz implementasyonu ayrı yapılır. |

---

## 1. Tasarım İlkeleri

| İlke | Uygulama |
|---|---|
| Superloop uyumlu | Tek giriş noktası `BMS_Run()`; hiçbir çağrı bloklamaz, `while` ile bekleme yok. |
| Non-polling | SDK veri beklemez, taramaz. Kaynak `BMS_Notify*` ile dirty bit diker; iş bir sonraki `BMS_Run()` turunda. Çıkış yönünde de aynı: BMS değişiklikte hook çağırır, connector `seq` kontrol etmez. Periyodik işler (stale kontrol, duty-cycle, NVM) deadline karşılaştırmalı yazılım timer'ları ile. |
| Zaman | Tek kaynak: `portFnMap.getTickMs`. Her ölçüm zaman damgalı; yaş buradan türetilir. |
| Bellek | `malloc` yok. Her şey statik; kapasiteler `BMS_CFG_*` ile (`BMS_UserConfig.h`). Diziler SDK'da kopyalanmaz, connector pointer'ından yerinde okunur. |
| Sürücü bağımsız | SDK hiçbir sürücü bilmez. Sürücü ↔ SDK bağı sadece connector (pointer'lar + hook'lar + Notify). Yeni sensör = yeni connector, core'a dokunulmaz. |
| Güvenlik | Her ölçümün yaş bilgisi var; stale veri = fault. Plausibility (hücre toplamı vs modül/pack V, birincil vs ikincil akım). Config CRC, watchdog hook. |
| Determinizm | Attach pointer kopyalar, Notify bayrak diker; hesap `BMS_Run()` içinde. Hook'lar bloklamaz (sürücü meşgulse connector biriktirir). Worst-case `BMS_Run()` süresi ölçülebilir. |

---

## 2. Entegrasyon Modeli: Neden "SDK saf motor"

**Model A (SDK sürücüyü çalıştırır):** Sürücü SDK vtable'ına sarılır, `BMS_Run()` sürücü FSM'ini ilerletir.

**Model B (connector):** Sürücüler kullanıcı superloop'unda kendi modeliyle çalışır. SDK sadece bağlı veriyi işler ve hook'larla setpoint uygular.

| Kriter | Model A | Model B |
|---|---|---|
| Mevcut BQ sürücüsü | SDK async modeline uyarlanmalı | Dokunulmaz, connector yeter |
| CAN akım sensörü / slave kart verisi | Zorla "sürücü" gibi sarılır | Doğal: sürücü tamponu bağlanır, frame gelince Notify |
| SDK bloklanma riski | Sürücü kötüyse SDK de bloklanır | Sıfır |
| Port katmanı | SPI/I2C/ADC/DMA async port gerekir | Sadece tick + crit + (ops.) NVM/log |
| Ölçüm zamanlaması | SDK kontrol eder | Dışarıda; SDK bayrak ve hook ile yönlendirir |
| Test | Mock port gerekir | Sadece Attach + Notify çağrıları |

Model A'nın tek avantajı (balancing kapalıyken ölçüm gibi zamanlama senkronu) Model B'de şöyle karşılanır: `BMS_FEED_FLAG_BALANCING_ACTIVE` bayrağı (connector bilir, SDK filtreler), SDK'nın duty-cycle ile mask'i zamanda açıp kapatması, `requestMeasurement` hook'u.

**Kopyalama:** 32 cihaz × 18 hücre = 1152 byte; kopyalamak CPU'da ~2 µs ama SDK'da 3-4 KB fazladan RAM ve her kaynak için kopya kodu demek. Connector modelinde kopya yok (bölüm 5.1).

---

## 3. Katmanlar ve Sınır

```
+----------------------------------------------------------------------+
|  Kullanici superloop                                                 |
|   for(;;) {                                                          |
|      BQ_AFE_Update();        /* surucu; parse -> eventFnMap        */|
|      _drainCanRx();          /* IVT_CAN_OnFrame -> eventFnMap      */|
|      SHUNT_ADC_Update();     /* -> eventFnMap                      */|
|      CONTACTOR_CONN_Update();/* feedback oku -> Notify             */|
|      BMS_Run();              /* SDK: 1 tur; setpoint -> hook'lar   */|
|      BQ_AFE_CONN_Update();   /* biriken mask -> surucu             */|
|      _telemetryUpdate();     /* ayri implementasyon, snapshot okur */|
|   }                                                                  |
+---------------------------+------------------------------------------+
|  CONNECTOR'LAR (uygulama tarafi, SDK ornek verir)                    |
|   BQ_AFE_Connector   Current_Connector   Contactor_Connector         |
|   init: BMS_Attach*Connector(pointer'lar + hook'lar)                 |
|   surucu callback'i: BMS_Notify*     BMS hook'u: surucuye komut      |
+======================= SDK SINIRI ===================================+
|  GIRIS   BMS_Attach*Connector / BMS_Notify*        (BMS_Connector.h) |
|  CIKIS   connector hook'lari / BMS_GetSetpoint / Snapshot / eventFnMap|
+----------------------------------------------------------------------+
|  BMS core : model, staleness, fault motoru, balancing, estimator,     |
|             limitler, kontaktor FSM, coklu pack agregasyonu          |
+----------------------------------------------------------------------+
|  BMS_PortFnMap_t : getTickMs, critEnter/Exit, (ops.) nvm, log        |
+----------------------------------------------------------------------+
```

SDK'nın derleme bağımlılığı: `stdint.h`, `stdbool.h`, `string.h`, `math.h` ve kullanıcının `BMS_UserConfig.h`'ı. Hiçbir HAL başlığı yok.

---

## 4. Dizin Yapısı

```
bms-sdk/
├── include/
│   ├── BMS.h                 # tek giris: Init, Run, komutlar, NVM blob
│   ├── BMS_Types.h           # birimler, roller, bayraklar, BMS_PortFnMap_t
│   ├── BMS_Cfg.h             # pack/limit/balance/fault/chemistry descriptor'lari
│   ├── BMS_Connector.h       # Monitor/Current/Contactor connector'lari, Attach, Notify
│   └── BMS_Out.h             # Setpoint, Snapshot, erisimciler, BMS_EventFnMap_t
├── src/
│   ├── BMS.c                 # Init, Run, handle (_hBms), komutlar
│   ├── BMS_Connector.c       # baglama tablolari, dirty bitler, ISR kuyrugu, hook cagrilari
│   ├── BMS_Model.c           # pack/module/cell durumu, min/max/delta, staleness, NTC donusumu
│   ├── BMS_Fault.c           # tablo tabanli debounce/hysteresis/severity   (BMS_FAULT_*)
│   ├── BMS_Balance.c         # pasif balancing, duty-cycle, termal kisit     (BMS_BALANCE_*)
│   ├── BMS_Limits.c          # CCL/DCL, max sarj V                           (BMS_LIMITS_*)
│   ├── BMS_Contactor.c       # precharge/main FSM (opsiyonel modul)          (BMS_CONTACTOR_*)
│   ├── BMS_System.c          # coklu pack agregasyonu, paralel baglama       (BMS_SYSTEM_*)
│   ├── BMS_Out.c             # setpoint olusturma, seq, hook + eventFnMap cagrilari
│   ├── BMS_Soc.c             # varsayilan: coulomb + OCV re-sync (kimya sonra) (BMS_SOC_*)
│   ├── BMS_Soh.c             # kapasite + DCIR                               (BMS_SOH_*)
│   ├── BMS_Sched.c           # yazilim timer'lari                            (BMS_SCHED_*)
│   └── BMS_Util.c            # CRC32, LUT interpolasyon, NTC                 (BMS_UTIL_*)
├── drivers/
│   └── BQ_AFE/               # mevcut BQ79718 surucusu (bugun repo kokunde, tasinacak)
├── examples/
│   ├── stm32_bq79718_stack/  # 32 cihaz, IVT, shunt, kontaktor (yazildi, sozdizimi dogrulandi)
│   └── host_sim/             # simule hucre modeli + sanal suruculer, tam dongu
├── tests/                    # Unity
├── docs/
└── CMakeLists.txt
```

---

## 5. SDK Sınırı

### 5.1 Giriş: Connector'lar (`BMS_Connector.h`)

Her connector bir struct: **veri bağları** (kaynağın kendi tamponlarına pointer) + **hook'lar** (BMS'in çağıracağı fonksiyonlar). `BMS_Attach*Connector` struct'ı kendi tablosuna kopyalar; struct geçici olabilir.

```c
typedef struct {
	const int16_t				*cellVoltagesMv;      /* [0] = hucre 1 */
	uint8_t						numOfCells;
	const int32_t				*moduleVoltageMv;     /* NULL olabilir */
	const BMS_DeviceStatus_t	*deviceStatus;        /* NULL olabilir */
	BMS_TempGroupBinding_t		tempGroups[BMS_CFG_MAX_TEMP_GROUPS_PER_MODULE];   /* values, slot, role, ntc, refMv */
	uint8_t						numOfTempGroups;
	void	(*applyBalanceMask)		(uint8_t moduleIndex, uint32_t cellMask, void *ctx);
	void	(*applyFetEnable)		(uint8_t moduleIndex, bool chg, bool dchg, void *ctx);
	void	(*requestMeasurement)	(uint8_t moduleIndex, void *ctx);
	void	(*clearDeviceFaults)	(uint8_t moduleIndex, void *ctx);
	void	*ctx;
}BMS_MonitorConnector_t;
BMS_Status_t BMS_AttachMonitorConnector(uint8_t packIndex, uint8_t moduleIndex, const BMS_MonitorConnector_t *connector);
BMS_Status_t BMS_NotifyMonitor(uint8_t packIndex, uint8_t moduleIndex, uint32_t dataMask, uint32_t tickMs, uint32_t flags);
/* dataMask: BMS_MONITOR_DATA_CELLS | _MODULE_VOLT | _STATUS | _TEMP_GROUP(n) */

typedef struct {
	const int32_t	*currentMa;            /* zorunlu */
	const int64_t	*chargeMas;            /* NULL: BMS entegre eder */
	const int32_t	*packVoltageMv, *busVoltageMv;
	const int16_t	*shuntTempDeciDeg;  uint16_t shuntTempSlot;
}BMS_CurrentConnector_t;
BMS_Status_t BMS_AttachCurrentConnector(uint8_t packIndex, BMS_CurrentSource_t source, const BMS_CurrentConnector_t *connector);
BMS_Status_t BMS_NotifyCurrent(uint8_t packIndex, BMS_CurrentSource_t source, uint32_t dataMask, uint32_t tickMs);
BMS_Status_t BMS_NotifyCurrentFromIsr(...);   /* int32 atomik; sadece bayrak kuyruga */

typedef struct {
	void		(*applyCommand)(BMS_ContactorCmd_t command, void *ctx);
	void		*ctx;
	const bool	*feedbackClosed[BMS_CFG_MAX_CONTACTORS];   /* 0 = main-, 1 = precharge, 2 = main+ */
	uint8_t		numOfFeedbacks;
}BMS_ContactorConnector_t;
BMS_Status_t BMS_AttachContactorConnector(uint8_t packIndex, const BMS_ContactorConnector_t *connector);
BMS_Status_t BMS_NotifyContactor(uint8_t packIndex, uint8_t contactorIndex, uint32_t tickMs);
```

Bağlama sözleşmesi: pointer init'ten sonra değişmez; tampon `BMS_Run()` ile aynı bağlamda (ana döngü) yazılır; yazıcı her tutarlı güncellemeden sonra Notify çağırır. ISR'dan sadece `BMS_NotifyCurrentFromIsr` (int32 atomik, bayrak kuyruğa, `critEnter/Exit` ile). Hook'lar `BMS_Run()` içinden çağrılır, bloklamaz; sürücü meşgulse connector isteği biriktirir ve kendi `Update`'inde yeniden dener.

İç mekanizma: Attach pointer tablosuna yazar, Notify dirty bit + tick diker. `BMS_Run()` sadece dirty olanlara dokunur; NTC dönüşümü, min/max taraması orada.

### 5.2 Çıkış: Hook'lar, Setpoint, Snapshot, Olaylar (`BMS_Out.h`)

Balance mask, kontaktör komutu, FET izinleri **BMS tarafından connector hook'larıyla uygulanır** (sadece değişen modül/komut için). `BMS_GetSetpoint` bilgi amaçlıdır (limitler, izinler; telemetri ve şarj cihazı için):

```c
typedef struct {
	uint32_t			seq;
	uint32_t			balanceMask[BMS_CFG_MAX_MODULES];
	bool				chgAllowed, dchgAllowed;
	int32_t				cclMa, dclMa, chgTargetMv;
	BMS_ContactorCmd_t	contactorCmd;
	bool				fetChgEnabled, fetDchgEnabled;
	BMS_Severity_t		severity;
}BMS_Setpoint_t;

const BMS_Setpoint_t     *BMS_GetSetpoint     (uint8_t packIndex);
const BMS_PackSnapshot_t *BMS_GetPackSnapshot (uint8_t packIndex);
int16_t                   BMS_GetCellVoltageMv(uint8_t packIndex, uint16_t cellIndex);   /* connector tamponundan yerinde */

typedef struct {
	void (*onSetpointChanged)(uint8_t packIndex, const BMS_Setpoint_t *setpoint);
	void (*onFaultRaised)    (uint8_t packIndex, uint8_t ruleId, BMS_Severity_t severity);
	void (*onFaultCleared)   (uint8_t packIndex, uint8_t ruleId);
	void (*onContactorState) (uint8_t packIndex, BMS_ContactorState_t state);
	void (*onNvmSaveRequest) (uint8_t packIndex);
}BMS_EventFnMap_t;
```

Tam örnek: `examples/stm32_bq79718_stack/` (`BQ_AFE_Connector.c`, `Current_Connector.c`, `Contactor_Connector.c`).

### 5.3 Komutlar (uygulama → SDK)
```c
BMS_Status_t BMS_RequestContactor  (uint8_t packIndex, BMS_ContactorReq_t request);
BMS_Status_t BMS_SetMode           (BMS_Mode_t mode);
BMS_Status_t BMS_ClearFaults       (uint8_t packIndex, uint32_t ruleMask);
BMS_Status_t BMS_SetBalanceEnabled (uint8_t packIndex, bool enabled);
BMS_Status_t BMS_RequestMeasurement(uint8_t packIndex);
```

---

## 6. Çekirdek Çalışma Modeli

```c
void BMS_Run(void) {

	uint32_t tickMs = _hBms.portFnMap.getTickMs();

	BMS_SCHED_Poll(tickMs);                 /* timer'lar: stale check, balance duty, nvm, soh */
	_drainIsrQueue();                       /* NotifyCurrentFromIsr -> dirty                  */

	for (each pack) {
		BMS_MODEL_Update(pack, tickMs);     /* dirty modullerde min/max/delta, NTC donusum, age */
		BMS_FAULT_Eval(pack, tickMs);       /* tablo: debounce, hysteresis, severity          */
		BMS_SOC_Update(pack, tickMs);       /* sadece yeni akim/hucre verisi varsa            */
		BMS_LIMITS_Eval(pack);              /* CCL/DCL                                        */
		BMS_BALANCE_Update(pack, tickMs);   /* mask hesapla (duty-cycle timer'a bagli)        */
		BMS_CONTACTOR_Update(pack, tickMs); /* FSM                                            */
		BMS_OUT_Build(pack);                /* degisen: seq++, hook'lar (applyBalanceMask     */
		                                    /* sadece degisen moduller, applyCommand),        */
		                                    /* onSetpointChanged                              */
	}

	BMS_SYSTEM_Update(tickMs);              /* agregasyon, paralel pack kontrolu              */
}
```
Her adım "dirty yoksa hemen dön" kuralıyla yazılır; boş turda maliyet birkaç yüz cycle. 32 modül × 18 hücre min/max taraması ~5 µs.

---

## 7. Veri Modeli ve Konfigürasyon

```
_hBms
 └── pack[0..P-1]
      ├── module[0..M-1]            -- 1 AFE = 1 modul; MonitorConnector baglari + hook'lar
      │     └── cellFlags[0..C-1]   -- BMS_CellFlags_t (1 byte); voltaj SDK'da KOPYALANMAZ
      ├── tempSlot[0..T-1]          -- {deciDeg onbellek, tickMs, role}; kaynak: temp group baglari
      ├── currentSrc[0..1]          -- CurrentConnector baglari, tickMs
      ├── contactor                 -- ContactorConnector hook + feedback baglari, FSM state
      ├── faultState[rule]          -- debounce sayaci, aktif, latched
      └── estimator state           -- SoC, SoH, DCIR, Ah, cycle
```

RAM (32 modül × 18 hücre): hücre bayrağı 576 B + modül bağ/agrega ~1.2 KB + sıcaklık önbelleği 640 B + pack ~200 B ≈ 2.6 KB.

Konfigürasyon (`const`, derleme zamanı): `BMS_PackCfg_t { numOfModules, cellsPerModule[], numOfTempSlots, numOfCurrentSources, chemistry, limits, balance, faultRules, stale, contactor }`, `BMS_SystemCfg_t { numOfPacks, packs[], parallelCloseDeltaMv }`. Örnek: `examples/stm32_bq79718_stack/App_Config.c`.

---

## 8. Alt Sistemler

### 8.1 Fault motoru (`BMS_FAULT_*`)
- Tablo: `BMS_FaultRule_t { id, signal, compare, threshold, hysteresis, debounceMs, severity, actions }`.
- Sinyaller (`BMS_Signal_t`): cell V max/min/delta, pack V, I chg/dchg, temp max/min (rol bazlı), AFE die, shunt temp, stale (her kaynak), current mismatch, sum vs module/pack V, **device fault** (AFE'nin kendi koruma bitleri, `BMS_COMPARE_ANY_BIT` ile maske).
- Severity: INFO → WARN → DERATE → TRIP. Action: limit chg/dchg, stop balance, open contactor, latch.

### 8.2 Balancing (`BMS_BALANCE_*`)
- Pasif. `vCell - vMin > deltaStartMv`, sadece şarjda/dinlenmede (konfigüre edilebilir).
- Duty-cycle timer ile mask zamanda açılır/kapanır; `applyBalanceMask` sadece değişen modüller için çağrılır. `BMS_FEED_FLAG_BALANCING_ACTIVE` ile gelen örnekler filtrelenir.
- Termal kısıt (AFE_DIE / CELL rolündeki sıcaklıklar). Komşu-hücre kısıtı `noAdjacentCells`. Modül başına `maxCellsPerModule`.

### 8.3 Estimator (SoC/SoH), takılabilir
Varsayılan `BMS_SOC_*`: coulomb sayımı (connector `chargeMas` veriyorsa onu tercih eder) + dinlenmede OCV re-sync. OCV tablosu kimya ile sonra gelir; şimdilik lineer yer tutucu. EKF istenirse aynı arayüzle yeni modül.

### 8.4 Limitler (`BMS_LIMITS_*`)
CCL/DCL: sıcaklık × SoC × cell V haritaları; DERATE çarpanı (`deratePct`). Çıktı setpoint'te, `onSetpointChanged` ile duyurulur.

### 8.5 Kontaktör FSM (`BMS_CONTACTOR_*`, opsiyonel)
OPEN → PRECHARGE → (busVolt ≥ packVolt × prechargeOkPct, t < timeout) → CLOSE_MAIN → CLOSED; hata → FAULT_OPEN (latch). Komut `applyCommand` hook'u ile. Feedback varsa weld detection. Entegre FET'li AFE kullananlar bu modülü kapatıp monitor connector'ın `applyFetEnable` hook'unu kullanır.

### 8.6 Çoklu pack (`BMS_SYSTEM_*`)
Sistem SoC Ah ağırlıklı, toplam akım, sistem CCL/DCL = paralel toplam (fault'lu pack hariç). Paralel bağlama: `|vPack_i - vPack_j| < parallelCloseDeltaMv` değilse CLOSE isteği reddedilir.

### 8.7 Kalıcılık
SDK blob verir (`BMS_NvmBlob_t`: SoC, SoH, mAs, cycle, kapasite, CRC32). `onNvmSaveRequest` olayında uygulama `BMS_ExportNvm` çağırıp kendi ortamına yazar.

---

## 9. Port: `BMS_PortFnMap_t`

Sürücünün `ioFnMap` deseniyle aynı: `BMS_Init`'e fonksiyon haritası verilir, `extern` ile beklenen port fonksiyonu yoktur.
```c
typedef struct {
	uint32_t	(*getTickMs) (void);
	void		(*critEnter) (void);
	void		(*critExit)  (void);
	bool		(*nvmRead)   (void *dst, uint32_t len);          /* NULL olabilir */
	bool		(*nvmWrite)  (const void *src, uint32_t len);    /* NULL olabilir */
	void		(*log)       (uint8_t level, const char *msg);   /* NULL olabilir */
}BMS_PortFnMap_t;
```

---

## 10. Birimler ve Tipler
- Saklanan ve iletilen her değer tam sayı: `int16_t` mV (hücre), `int32_t` mV (pack), `int32_t` mA (+ şarj), `int16_t` deciDeg (0.1 °C), `int64_t` mAs, `uint16_t` 0.01 % (SoC/SoH), `uint32_t` ms (wrap-safe karşılaştırma).
- Aritmetik float: LUT interpolasyonu, NTC dönüşümü, limit haritaları, estimator hesapları `float` ile yapılır, sonuç `lroundf` ile tam sayıya yazılır.
- Bir fonksiyonun imzasında veya struct'ta float görünmez.

---

## 11. Test Stratejisi
- **Unit (host, Unity + CMake)**: her modül; Attach → Notify → Run → hook/Snapshot zinciri. Port fn map'te sadece sahte tick. Hook'lar test içinde sayaç/kayıt fonksiyonları.
- **Simülasyon (`examples/host_sim`)**: hücre modeli (OCV + R + RC) + sanal AFE sürücüsü (eventFnMap ile) + sanal akım sensörü. Doğrulanan: stale fault, debounce, balancing delta kapanması, SoC yakınsaması, precharge FSM, paralel pack bağlama reddi.
- **Hedef**: STM32 + mevcut BQ sürücüsü + connector'lar. `BMS_Run()` worst-case süresi DWT ile ölçülür.

---

## 12. Yol Haritası

| Faz | Kapsam | Çıktı |
|---|---|---|
| 0 | Bu klasörde `git init` + `.gitignore`, CMake, Unity, `BMS_UserConfig.h` şablonu | derlenen boş SDK + 1 test |
| 1 | `BMS_Connector.c` (bağ tabloları, dirty bitler, ISR kuyruğu), `BMS_Sched.c`, `BMS_Model.c` (min/max/delta/age/NTC), `BMS_Out.c` (seq, hook'lar, eventFnMap) | Attach → Notify → Run → hook/Snapshot zinciri testte |
| 2 | Fault motoru (stale, eşik, plausibility, device fault), limitler | tablo tabanlı testler |
| 3 | Balancing (duty-cycle, bayrak filtresi, termal, `applyBalanceMask` sadece değişen modüller) | sim'de delta kapanıyor |
| 4 | Kontaktör FSM (`applyCommand`), komutlar, çoklu pack agregasyonu | 2 pack sim |
| 5 | Estimator arayüzü + CC/OCV varsayılan (yer tutucu kimya), SoH basic, NVM blob | sim'de yakınsama |
| 6 | Gerçek sürücü entegrasyonu: `BQ_AFE` D1-D8 + connector'lar, STM32 örneği linklenir | donanımda çalışan 1 pack |
| 7 | Dokümantasyon, API freeze (v1.0) | release |
| sonra | Kimya/OCV tabloları, EKF, aktif balancing | |

Sürücü hattı (D1-D10, [BQ_AFE_REVIEW.md](BQ_AFE_REVIEW.md) bölüm 5) SDK fazlarına paralel yürür. D1-D5 hemen başlayabilir; D10 (connector) SDK Faz 3 ile buluşur.

---

## 13. Kalan Açık Noktalar
1. Repo: `batteryManagerSdkTrials/` bugün kendi git deposu değil; üst dizin (`C:\Users\serve`) bir depo ve bu klasör orada untracked görünüyor. Faz 0'da bu klasörde `git init` + `.gitignore` yapılmalı.
2. CAN akım sensörü hangi model (IVT-S/IVT-MOD?), As sayacı frame'i var mı? Varsa estimator onu CC kaynağı yapar.
3. BQ79718'de entegre FET yok; kontaktör modülü varsayılan açık. Çipin CS ADC ile akım ölçüp ölçmediği doğrulanacak (REVIEW bulgu 15).
4. Stack'te kaç BQ79718 ve pack başına hücre sayısı? `BMS_CFG_MAX_MODULES`, `BMS_CFG_MAX_CELLS_PER_MODULE = 18` üst sınırlarını belirler.
5. Termistör: GPIO kanal → slot eşlemesi, NTC beta/tablosu, TSREF voltajı. Connector konfigürasyonunda (`BQ_AFE_CONN_Cfg_t`).
6. IVT frame yerleşimi (0x521 tabanlı, big-endian int32) ve `cbTimerCode` değeri datasheet'ten doğrulanacak.
