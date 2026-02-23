# ESP32 Web Architecture

## Current shape (today)

The firmware entrypoint is concentrated in [main/main.cpp](main/main.cpp). It currently contains:

- Wi-Fi connection lifecycle and retry handling
- GPIO target-specific capability and protection logic
- WebSocket protocol parsing and command handling
- HTTP endpoint registration and static asset serving
- Snapshot serialization and polling loop

This works, but it couples transport, domain logic, and platform concerns in one compilation unit.

## Why modularize

Moving to modular components gives:

- Better separation of concerns (each module owns one responsibility)
- Easier multi-target support (esp32s3, esp32c6, etc.)
- Less duplicated logic and safer changes (DRY)
- Better testability (especially for parsing and pin policy)

## Proposed module boundaries

Use domain-first organization under `internal/` and keep platform wiring in `main/`:

- `internal/board/`
	- target profile and protected-pin policy
	- capabilities derived from SOC mask
- `internal/gpio/`
	- pin state model, mode changes, read/write, snapshot projection
- `internal/wifi/`
	- connect/reconnect lifecycle, status events, IP readiness
- `internal/web/`
	- HTTP route registration and embedded asset responses
- `internal/ws/`
	- message decoding/validation and command dispatch
- `internal/common/`
	- shared JSON keys, constants, and small utilities

`main/main.cpp` should only compose services and run the event loop.

## Dependency direction (important)

- `main` depends on `internal/*`
- `internal/web` and `internal/ws` depend on interfaces from `internal/gpio` and `internal/wifi`
- domain modules do not depend on UI/web asset details

This keeps core behavior reusable and open for extension.

## Refactor plan (safe, incremental)

1. Extract board/pin policy
	 - move protected pin logic and pin-state creation from [main/main.cpp](main/main.cpp) into `internal/board` + `internal/gpio`.
2. Extract WebSocket command parser
	 - move `set/write/snapshot` parsing and validation into `internal/ws`.
3. Extract Wi-Fi lifecycle
	 - isolate connect/retry/events and expose a simple status interface.
4. Extract HTTP/web server wiring
	 - move endpoint handlers and embedded file serving to `internal/web`.
5. Compose from main
	 - keep only startup composition in [main/main.cpp](main/main.cpp).

Each step should preserve current behavior and be validated with build + flash + monitor.

## DRY opportunities already visible

- Centralize websocket message keys (`type`, `gpio`, `mode`, `value`) in one constants unit.
- Consolidate repeated JSON payload creation for errors/snapshots.
- Keep target-specific pin reservations in one board profile source.

## Multi-target strategy

- Keep target defaults in:
	- [sdkconfig.defaults.esp32c6](sdkconfig.defaults.esp32c6)
	- [sdkconfig.defaults.esp32s3](sdkconfig.defaults.esp32s3)
- Keep loading logic in [CMakeLists.txt](CMakeLists.txt).
- Keep frontend pin rendering dynamic from firmware snapshot (already done in [main/web/app.js](main/web/app.js)).

## Definition note

- In architecture discussions here, "SoC" means separation of concerns.
- In firmware/platform discussions, "SoC capabilities" means hardware capabilities of the chip (e.g., valid GPIO mask).

