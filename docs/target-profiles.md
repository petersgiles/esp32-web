# Target-specific ESP-IDF defaults

This project now loads defaults in layers:

1. `sdkconfig.defaults` (shared settings for all chips)
2. `sdkconfig.defaults.<target>` (target-specific settings), for example:
   - `sdkconfig.defaults.esp32c6`
   - `sdkconfig.defaults.esp32s3`

`CMakeLists.txt` selects the second file automatically using `IDF_TARGET`.

## Why this helps

When you switch targets (for example `esp32s3` to `esp32c6`), each chip can keep its own partition/profile defaults without manually editing `sdkconfig` every time.

## Current profile choices

- `esp32c6`: `CONFIG_PARTITION_TABLE_SINGLE_APP_LARGE=y`
- `esp32s3`: `CONFIG_PARTITION_TABLE_SINGLE_APP_LARGE=y`

This avoids app-size overflows like:

- `factory ... size 0x100000 (overflow ...)`

## Typical workflow

1. Set target
   - `idf.py set-target esp32c6`
   - or `idf.py set-target esp32s3`
2. Reconfigure/build
   - `idf.py build`
3. Flash/monitor using your selected serial port (`/dev/ttyACM0`, etc.)

## Notes

- `sdkconfig` is generated/updated by ESP-IDF and can differ per machine.
- Keep long-lived intent in `sdkconfig.defaults*` files, not by manually editing generated `sdkconfig`.
