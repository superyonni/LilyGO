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

### Codec (pegar el JavaScript que ya tienes)
Decodifica 12 bytes: lat, lon, alt, batería.

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

## Checklist de errores comunes

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
