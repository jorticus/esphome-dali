The ESPHome custom component uses the MIT-licensed `esp-idf-dali` RMT driver for
DALI transmit and receive. Configure the ESP32 with the ESP-IDF framework.
The driver is fetched from the `master` branch of
`github.com/ferdinandkeil/esp-idf-dali` (see `DALI_LIB_REPOSITORY` in
`__init__.py`), which provides ESP-IDF 5.x RMT support, a PlatformIO manifest and
`dali_init_with_inversion()`, used here with non-inverted TX for the board's
inverting transistor driver. The component adapts the driver's transaction
API to the DALI port interface used for lights and address discovery. The
driver's `resetBus()` pulse operation is not available through its RMT API.

The separate PlatformIO library/demo remains Arduino-compatible and builds the
core DALI protocol code independently of the ESPHome component.
