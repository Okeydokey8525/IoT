# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

HUIT IoT course project, Nhóm 16, Đề tài 21: ESP32 system that monitors household appliance power and protects against overload. Documentation, comments and serial logs are in Vietnamese (code comments/logs are written without diacritics). The project is simulated on Wokwi — there is no physical hardware; a potentiometer stands in for the load.

## Layout (root is both the PlatformIO project and the git repo)

- `src/main.cpp` — the single firmware source. The team demos on **Wokwi web** (the T1–T8 evidence came from there): the same file is pasted into the web project's `sketch.ino`. It must compile on both PlatformIO (arduino-esp32 core 2.x) and Wokwi web (core 3.x) — the buzzer LEDC calls are split with `#if ESP_ARDUINO_VERSION_MAJOR >= 3`. After editing, remind the user to paste it back to Wokwi web.
- `diagram.json` — copied from the Wokwi web project (same pins as the code).
- `node-red/flows.json` — Node-RED flow export (layers 3–4).
- `docs/` — `context-handoff.md` (4-layer architecture, Node-RED details, weekly progress, week-12 tasks), `syllabus-37-de-tai.docx` (topic 21 requirements), `bao-cao-tuan/`, `slides/`.
- `images/` — T1–T8 evidence screenshots referenced by `README.md`.
- `Bao_cao/` — Word course report, built with the `bao-cao-word-huit` skill: chapter content in `md/00_PHAN_DAU.md` … `06_KET_LUAN.md` (edit Markdown, never the generated .docx), template `mau_word/Mau_SportHub.docx` (another course's finished report, used only for styles/cover), scripts in `py/`, figures in `hinh/` (`hinh_x_y.png`), draw.io sources in `so_do/`. Build: `cd Bao_cao && python3.13.exe py/build_bao_cao_iot.py`, then update TOC and export PDF via Word COM (see the skill). Diagrams 3.1 and 3.3 come from `py/ve_so_do_drawio.py`. Placeholders `[ĐIỀN …]` and missing `hinh_4_1`, `hinh_5_*` (evidence to be re-shot with the EMQX firmware) are intentional.
- MQTT broker shared by firmware and Node-RED (since 2026-11-24): private **EMQX Cloud Serverless** deployment `a1121f66.ala.us-east-1.emqxsl.com:8883`, TLS only, username/password required (accounts `esp32_nhom16`, `nodered_nhom16`; free tier, Spend Limit $0). Firmware reads host/user/pass from `src/secrets.h` (gitignored; template `src/secrets.h.example`; on Wokwi web it is an extra `secrets.h` tab) and pins the DigiCert Global Root CA (expires 2031-11-10). Never print or commit the passwords. Before 2026-11-24 the public `broker.emqx.io:1883` was used (old evidence images show it). README evidence text mentions "HiveMQ" because the HiveMQ web client was used to view messages.

Topic 21 per syllabus: "Hệ thống giám sát điện năng và tự ngắt máy móc khi quá tải"; since 2026-11-25 the team's title on the report cover and slides is "Hệ thống giám sát và cảnh báo quá tải điện năng thông minh" — scoped by the team to household (dân dụng) appliances, not industrial. Known gaps vs. the syllabus: current/voltage are not measured directly (pot simulates P), no instant short-circuit trip, no Home Assistant.

## Commands (run at the repo root)

```bash
pio run                                                        # build -> .pio/build/esp32dev/firmware.bin/.elf
pio device monitor --port rfc2217://localhost:4444 --baud 115200   # serial via Wokwi RFC2217 (port set in wokwi.toml)
```

Simulation: VS Code Wokwi extension → open `diagram.json` → "Wokwi: Start Simulator" (loads the firmware paths from `wokwi.toml`). There are no automated tests; verification is the manual T1–T8 matrix in `README.md`, observed via the serial log and an MQTT client subscribed to `nhom16/bai21/#`.

## Firmware architecture (`src/main.cpp`)

Two threads (split on 2026-11-20 after T5 showed `mqtt.connect()` to an unreachable broker blocks ~5 s per attempt, which delayed the trip when everything shared one loop):
- **`protectTask`** (FreeRTOS, core 1, priority 3, `vTaskDelayUntil` every 10 ms): sampling + FSM + LED/buzzer/relay. It must never call any network function.
- **`loop()`** (network thread): Wi-Fi/MQTT, telemetry, alarm publish, serial logging. Blocking network calls are acceptable here only.
- Cross-thread communication goes through flags only: `pendingCmd` (loop → task), `alarmPending`/`alarmPowerW` (task → loop), and an 8-entry ring buffer of FSM transitions (`evtBuf`) printed by loop as `[FSM] t=<ms>: A -> B (P = ...)`. Shared measurements are read/written under `portENTER_CRITICAL(&dataMux)`. Only `protectTask` calls `setRelay`/`setBuzzer` (except initial setup).

protectTask each tick:
1. Read ADC on GPIO34, 8-sample moving average, `P = avg/4095 * 3000 W`, `I = P/220` (0 when relay is off), integrate energy E (kWh) only while the relay is closed. Then apply any `pendingCmd`.
2. FSM in two passes — **2A transition** then **2B outputs** in the same tick (deliberately split so the logged state and relay state never disagree):
   - NORMAL → WARNING when `P > 1200 W` (LED on, relay stays on)
   - WARNING → CRITICAL after 3 s above threshold (buzzer 1500 Hz, relay off, sets `alarmPending` once)
   - WARNING/CRITICAL → NORMAL only when `P < 1100 W` (hysteresis)

loop(): `handleNetwork()` (Wi-Fi then MQTT reconnect every 5 s; LWT `offline` retained on `status`, publishes `online`, subscribes `control`) → print FSM events → publish pending alarm when connected (late if offline) → every 2 s publish telemetry and print one status line (`\r\n` line endings are required in Wokwi's terminal).

MQTT topics (`nhom16/bai21/...`): `telemetry` (`{U,I,P,E,relay,state}`), `alarm` (edge-triggered via `alarmSent`/`alarmPending`, exactly once per CRITICAL entry), `control` (`{"cmd":"relay_on"|"relay_off"|"reset"}`), `status` (LWT). JSON via ArduinoJson 6 `StaticJsonDocument`; MQTT buffer raised to 512 bytes.

Control priority rules (the Node-RED dashboard depends on these):
- `relay_on`/`relay_off` set `manualRelayMode = true`, so the FSM stops re-closing the relay in NORMAL/WARNING.
- CRITICAL always overrides manual mode and cuts the relay.
- Recovering from CRITICAL, or `reset`, clears `manualRelayMode` and hands control back to the FSM.

Pins: pot GPIO34, LED GPIO25, buzzer GPIO26 (LEDC channel 0), relay GPIO33 (active-HIGH: HIGH = closed/powered). Keep `diagram.json` in sync when changing pins.

Known pitfalls already fixed and documented in `README.md`: `noTone()` triggering "LEDC is not initialized" on core 2.x (hence raw LEDC), serial port 4000 conflict (hence 4444), and missing `$serialMonitor` wires in `diagram.json`.

## Links

- Wokwi project: https://wokwi.com/projects/477826305121504257
- Node-RED editor (runs in Docker locally): http://localhost:1880/#flow/256994368d45d3ad (tab "Điện")
- Node-RED dashboard: http://localhost:1880/ui

## Node-RED (layer 3–4)

`mqtt in` telemetry → `json` → function "Tách U, I, P" with 5 outputs (P, U, I, E, state) → gauges/chart/text widgets; a `switch` with **numeric** `> 1200` comparison drives overload alerts (a string comparison caused false alarms before); a function computes EVN tiered electricity cost in VNĐ; a 1 msg/30 s rate-limited branch appends CSV rows to `/data/power_log.csv` (Docker volume). Two `ui_button`s publish `relay_off`/`relay_on` to `control`. Dashboard at `http://localhost:1880/ui`.

Security (report section 3.8): `/data/settings.js` in the container has `adminAuth` (editor) and, since 2026-11-24, `httpNodeAuth` (Dashboard) reusing the same user `admin` and bcrypt hash; backup of the previous settings in `_Luu_Tru_Ngoai/nodered_settings.*.js`. Headless screenshots of `/ui` now need basic-auth. The MQTT layer uses the private EMQX Cloud broker with TLS + accounts; only device-level security (flash encryption, secure boot) remains a documented limitation.
