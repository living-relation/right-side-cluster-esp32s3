# Right cluster — flash readiness

**Source tree (flash from here):** `C:\projects\right-side-cluster-esp32s3`  
**Git HEAD:** run `git rev-parse --short HEAD` after pull  
**Mode:** bench **OFF** — live UART from center (`# CONFIG_TC_BENCH_MODE is not set`)

## Unwired bench — flash this board alone

Plug **only** the right cluster via USB-C.

## Morning checklist

1. **ESP-IDF 5.4.2**
2. **USB** — right board only; note COM port (example: **COM8** — yours may differ)
3. **Target** — `esp32s3`
4. **Build:**
   ```powershell
   cd C:\projects\right-side-cluster-esp32s3
   .\scripts\prepare_flash.ps1
   ```
5. **Flash:**
   ```powershell
   .\scripts\flash_cluster.ps1 -Port COM8
   ```

## What to expect (bench off, unwired)

- Toyota splash (stable hold after fade-in; logo fades to black, then cut to gauges)
- Lambda + bar gauges at **0** — **no demo sweep**
- ECU warn overlay (`ui_warning.c`) only for **0x3EE bytes 0–5** (KNOCK, cuts, SENSOR, THROTTLE)
- Boost yellow BG flash at **≥28 PSI** (planned ECU threshold is 27 — backlog)

## Boot splash notes (RGB tear fix)

- Live UI widgets stay **hidden** until handoff; LVGL invalidation is **frozen** during the hold.
- Fade-out does **not** dissolve through semi-transparent overlay over gauges (that tore on this panel).
- Display path: **80-line partial buffer** + `bb_mode` (same as left S3).

## Settings (already in `sdkconfig.defaults`)

| Item | Value |
|------|--------|
| Target | esp32s3 |
| Flash | 16 MB |
| PSRAM | Octal 80 MHz |
| LVGL buffer | 80-line partial RGB |
| `CONFIG_LCD_RGB_ISR_IRAM_SAFE` | y (cmake auto-patch) |
| Bench mode | **off** |

## After you wire the car — reflash?

**No**, if this board already has the latest **bench-off** firmware. Connect **5 V**, **GND**, and
**GPIO44 ← center GPIO21** only. Reflash when you update firmware from git.

## Not in this firmware yet

Strobe/orange TC overlay, extended warn bytes — see `st185-furyx-base-map\docs\CLUSTER_FIRMWARE_BACKLOG.md`.
