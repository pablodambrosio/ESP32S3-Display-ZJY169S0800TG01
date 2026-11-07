# Agentic Specifications (`spec/`)

This directory contains specifications, architectural designs, and requirements for agentic and pair-programming development workflows.

## Specification Guidelines

When planning or implementing new modules or features, write a corresponding specification file in this directory following standard numbering and naming:

- Format: `spec/XXX-<topic-or-feature>.md` (e.g., `001-spi-display-driver.md`, `002-backlight-pwm.md`, `003-lvgl-port.md`)

---

## Active Specifications Index

| Spec ID | Title | Status | Description |
|:---:|---|:---:|---|
| [`001`](001-board-display-bringup.md) | **Board & Display Bring-up** | **Passed** | ESP32-S3-Zero init, safe binary GPIO backlight (`ON`/`OFF`), ST7789V2 `esp_lcd` driver, coordinate offsets ($X=0, Y=20$), and visual color/border tests. |
| [`002`](002-freertos-multicore-strategy.md) | **FreeRTOS & Multicore Strategy** | Ready for Implementation | Dual-core CPU pinning (Core 0 for I/O & logic, Core 1 for graphics & flush), double-buffered DMA ping-pong, thread-safe command queues, and task priorities. |
| [`003`](003-backlight-pwm-dimming.md) | **Backlight PWM Dimming (Optional)** | Pending Spec 001 Electrical Check | Optional LEDC PWM brightness (1 kHz, 10-bit) and hardware fade extension on `GPIO 9` (`BLK`) once `U2` gate/base current is verified. |

### Recommended Specification Structure

1. **Objective & Scope**: What does this feature accomplish? What are the boundaries?
2. **Hardware & Electrical Constraints**: Pins, bus speeds, voltage, memory limits (SRAM vs PSRAM).
3. **Software Architecture & Interfaces**: C API headers, structs, functions, error codes, and thread safety.
4. **Implementation Steps**: Phased breakdown of code changes across components and files.
5. **Verification & Testing Criteria**: How will the agent and developer test the feature on hardware or simulator?
6. **Rollback & Edge Cases**: Watchdog timeouts, SPI bus contention, memory exhaustion strategies.

