# ChirpStack ABP — paso a paso (techo-01)

OTAA falla con -1116 porque ChirpStack **no envía JoinAccept**.
ABP **no necesita join**: envías uplink directo.

Firmware ya configurado en modo ABP (`LORAWAN_USE_ABP 1`).

---

## PASO 1 — Perfil LilyGO-PROF

`Device profiles` → **LilyGO-PROF** → pestaña **Join (OTAA / ABP)**

| Campo | Valor |
|-------|-------|
| **Device supports OTAA** | **OFF** |

**Submit**

> Con OTAA=ON la pestaña Activation solo muestra el banner azul.
> Con OTAA=OFF aparece el **formulario** para activar.

---

## PASO 2 — Activar dispositivo techo-01

`Applications` → `LilyGO-TEcho` → `Devices` → **techo-01** → **Activation**

Pega **exactamente** (MSB):

| Campo | Valor |
|-------|-------|
| Device address | `260b1a2b` |
| Network session key | `a1b2c3d4e5f60718293a4b5c6d7e8f90` |
| Application session key | `0102030405060708090a0b0c0d0e0f10` |
| Uplink frame-counter | `0` |
| Downlink frame-counter | `0` |

**Submit**

Debe desaparecer el banner "not yet been activated".

---

## PASO 3 — Configuration

`techo-01` → **Configuration**

| Campo | Valor |
|-------|-------|
| Device EUI | `0a84041d4e2678ab` |
| Device profile | LilyGO-PROF |
| Device is disabled | OFF |
| **Disable frame-counter validation** | **ON** |

**Submit**

---

## PASO 4 — Subir firmware y probar

```powershell
cd C:\Users\Project-Manager\LilyGO
pio run -t upload
pio device monitor --port COM8
```

Pulsa **RESET**. Debes ver:

```
Modo ABP
DevAddr=0x260b1a2b
[LoRa] ABP activo DevAddr=0x260b1a2b
*** ABP OK - enviando uplink ***
[LoRa] Uplink OK ...
```

En ChirpStack → **techo-01** → **Events**: uplink FPort 1.

---

## Si reiniciaste y NO ves uplinks en Events

Cada **reset o reflash** del T-Echo reinicia el **frame-counter (FCnt)** a 0.
ChirpStack puede **rechazar** uplinks si el contador del servidor es mayor.

**Solución A (recomendada en pruebas):**

`techo-01` → **Configuration** → **Disable frame-counter validation = ON** → Submit

**Solución B:**

`techo-01` → **Activation** → poner **Uplink frame-counter = 0** → Submit

En el monitor serial debe aparecer en ~15 s del arranque:

```
[STATE] JOINING
[LoRa] ABP activo DevAddr=0x260b1a2b
[STATE] TRANSMIT
[LoRa] FCntUp=1
[LoRa] Uplink OK RSSI=...
```

Si ves `Uplink fallo: -XXXX` o no llega a TRANSMIT, copia el log completo.

---

## Si no hay uplink en Events

| Revisar | Acción |
|---------|--------|
| Activation no guardada | Submit de nuevo con las claves |
| FCnt rechazado | Disable frame-counter validation ON |
| Claves distintas | Copiar de nuevo desde esta guía |
| Gateway otro tenant | Mismo tenant que techo-01 |

---

## Volver a OTAA después

1. Perfil → OTAA **ON**
2. OTAA keys → AppKey `97a3251b063e6d91401167f8f97860c4`
3. `config.h` → `LORAWAN_USE_ABP 0`
4. Recompilar y subir
