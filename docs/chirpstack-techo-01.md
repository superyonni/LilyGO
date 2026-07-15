# ChirpStack — configuración exacta para techo-01 (T-Echo)

Guía basada en la documentación oficial:
- https://www.chirpstack.io/docs/guides/connect-device.html
- https://www.chirpstack.io/docs/chirpstack/use/devices.html
- Firmware RadioLib: https://github.com/jgromes/RadioLib/tree/master/examples/LoRaWAN

---

## Credenciales (copiar tal cual en ChirpStack y firmware)

| Campo | Valor | Byte order |
|-------|-------|------------|
| **DevEUI** | `0a84041d4e2678ab` | MSB |
| **JoinEUI** (AppEUI) | `0000000000000000` | MSB |
| **AppKey** | `97a3251b063e6d91401167f8f97860c4` | MSB |

> RadioLib usa MSB para todo. NO invertir DevEUI (eso es solo LMIC).

---

## PASO 1 — Device profile `LilyGO-PROF`

`Tenants` → `ChirpStack` → `Device profiles` → **LilyGO-PROF** → **Submit** en cada pestaña:

### General
| Campo | Valor |
|-------|-------|
| Region | AU915 |
| Region configuration | AU915 (channels 0-7 + 64) |
| MAC version | **LoRaWAN 1.0.3** |
| Regional parameters revision | A |
| ADR algorithm | Default ADR algorithm |
| Flush queue on activate | ON |

### Join (OTAA / ABP) — CRÍTICO
| Campo | Valor |
|-------|-------|
| **Device supports OTAA** | **ON** ← obligatorio para tu firmware |
| RX1 delay | 0 (default) |
| RX1 data-rate offset | 0 |
| RX2 data-rate | 0 |
| RX2 frequency | 0 |

### Codec (pegar desde `docs/chirpstack-codec.js`)

Decodifica 12 bytes big-endian: lat, lon, alt, batería.

**Importante:** el codec anterior fallaba con coordenadas en el hemisferio sur (JavaScript y `<< 24`). El archivo `docs/chirpstack-codec.js` usa `>>> 0` y corrige lat/lon.

Ejemplo: payload `50f399R8s0sALw/4` debe decodificar aprox. **lat -41.47, lon -73.00** (no -470 / -502).

### Measurements (opcional, solo gráficos)

La pestaña **Measurements** NO afecta la decodificación (eso es **Codec**). Sirve para gráficos en el dashboard.

Con **Automatically detect measurement keys = ON** (recomendado), tras el primer uplink con el codec corregido aparecerán las claves.

O añadir manualmente:

| Key | Kind |
|-----|------|
| latitude | Gauge |
| longitude | Gauge |
| altitude_m | Gauge |
| battery_v | Gauge |
| battery_mv | Gauge |
| gps_valid | String |

Pulsar **Submit**.

---

## PASO 2 — Application

`Applications` → **LilyGO-TEcho** (ya existe).

---

## PASO 3 — Device `techo-01`

`Applications` → `LilyGO-TEcho` → `Devices` → **techo-01**

### Configuration
| Campo | Valor |
|-------|-------|
| Name | techo-01 |
| Device EUI | `0a84041d4e2678ab` |
| Join EUI | `0000000000000000` |
| Device profile | LilyGO-PROF |
| Device is disabled | OFF |
| Disable frame-counter validation | OFF (por ahora) |

### OTAA keys — CRÍTICO
| Campo | Valor |
|-------|-------|
| Application key (AppKey) | `97a3251b063e6d91401167f8f97860c4` |
| Byte order | **MSB** |
| Gen App Key | dejar en ceros |

Pulsar **Submit**.

Luego pulsar **Flush OTAA device nonces** (cada vez que reprogrames o reinicies mucho el nodo).

### Activation
Con OTAA correcto, esta pestaña se llena **solos** tras un join exitoso.
No hace falta activar manualmente.

---

## PASO 4 — Gateway

El gateway debe estar en el **mismo tenant** que el dispositivo.

En `Gateways` → tu gateway → **LoRaWAN frames**:
- Debes ver **JoinRequest** seguido de **JoinAccept**
- Si solo hay JoinRequest → AppKey incorrecta o perfil con OTAA OFF

---

## PASO 5 — Firmware T-Echo

En `include/config.h` (ya configurado):

```c
#define LORAWAN_USE_ABP 0
#define LORAWAN_JOIN_EUI  0x0000000000000000ULL
#define LORAWAN_DEV_EUI   0x0A84041D4E2678ABULL
#define LORAWAN_APP_KEY   0x97, 0xA3, 0x25, 0x1B, ...
#define LORAWAN_SUBBAND 2
```

Subir y monitorear:

```powershell
cd C:\Users\Project-Manager\LilyGO
pio run -t upload
pio device monitor --port COM8
```

Pulsar RESET. Esperar:

```
Modo OTAA | AU915 subbanda 2 | join DR2
[LoRa] JOIN exitoso DevAddr=0x...
*** JOIN OK - enviando uplink de prueba ***
```

---

## Diagnóstico con tu log actual (JoinRequest OK, -1116 en serial)

Tu JoinRequest es **correcto**:
- DevEUI: `0a84041d4e2678ab` ✓
- JoinEUI: `0000000000000000` ✓
- SF10 @ 917.6 MHz ✓
- RSSI -17 / SNR 13 ✓ (señal excelente)

Error **-1116** = el T-Echo **no recibió JoinAccept** en Rx1/Rx2.

### Paso crítico AHORA (30 segundos)

Abre **dos pestañas** al mismo tiempo justo cuando el T-Echo intenta join:

1. **Gateway** `ac1f09fffe10a299` → LoRaWAN frames
2. **techo-01** → LoRaWAN frames

| Qué ves | Significado | Acción |
|---------|-------------|--------|
| Gateway: JoinRequest, **sin** JoinAccept | ChirpStack rechaza el join | Ver abajo "Servidor" |
| Gateway: JoinRequest **+** JoinAccept | Servidor OK, falla RX del nodo | Ver abajo "Downlink" |
| JoinRequest en gateway pero **no** en techo-01 | DevEUI no coincide en ChirpStack | Revisar Configuration |
| Events muestra **MIC error** | AppKey incorrecta | OTAA keys + Submit + Flush nonces |

### Servidor (sin JoinAccept en gateway)

1. Perfil **LilyGO-PROF** → Join → **OTAA = ON** → Submit
2. **techo-01** → OTAA keys → AppKey `97a3251b063e6d91401167f8f97860c4` MSB → Submit
3. **Flush OTAA device nonces**
4. **techo-01** → Configuration → Join EUI `0000000000000000` → Submit
5. Reinicia T-Echo (RESET)

### Downlink (JoinAccept SÍ en gateway, -1116 en serial)

- Gateway debe soportar **TX downlink** en AU915 (no solo RX)
- Acerca el T-Echo al gateway (< 5 m para prueba)
- Firmware ya usa RX boosted gain

---

| Síntoma | Causa | Solución |
|---------|-------|----------|
| JoinRequest en gateway, sin JoinAccept | Perfil OTAA OFF o AppKey distinta | OTAA ON + verificar AppKey MSB |
| JoinRequest en gateway, nada en device frames | DevEUI distinto en ChirpStack | `0a84041d4e2678ab` exacto |
| Error -1116 en serial | Sin JoinAccept (downlink) | Arreglar servidor; Flush nonces |
| MIC error en Events | AppKey no coincide | Regenerar o copiar de nuevo |
| Join OK pero sin uplink en Events | Codec/FCnt | Ver LoRaWAN frames del device |

---

## Referencias GitHub

| Repo | Uso |
|------|-----|
| https://github.com/chirpstack/chirpstack | Servidor LoRaWAN |
| https://github.com/jgromes/RadioLib | Librería firmware |
| https://github.com/radiolib-org/radiolib-persistence | Persistir nonces (producción) |
| https://github.com/Xinyuan-LilyGO/LilyGO-T-Echo | Placa T-Echo |
