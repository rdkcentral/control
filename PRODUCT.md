# rdkcentral/control Product Functionality

This document summarizes the core product features, capabilities, and user-facing functionality provided by the `rdkcentral/control` project.

## 1. Product Overview

`rdkcentral/control` provides the central control and management solution for RDK-based devices, focusing on remote control integration, device state management, configuration handling, and hardware abstraction. It enables flexible, reliable control of device functions, typically in set-top boxes and smart TV platforms.

## 2. Key Functional Areas

### 2.1 Remote Control Manager

- **Multi-protocol Support:** Integrates with RF4CE and BLE-based remote controls, supporting automatic pairing and discovery of devices.
- **Key Mapping & Input:** Manages IR, RF, and BLE key events, including vendor-specific key mappings (e.g., for AMC App).
- **Telemetry & Logging:** Captures and processes telemetry data from voice streams, key events, and audio sessions for diagnostics and monitoring.
- **OTA & Firmware Management:** Handles remote control firmware updates and network discovery, with stability features when interrupted (e.g., OTA interrupted by reset).

### 2.2 Device State & Power Management

- **Deep Sleep & Wake Handling:** Responds to device state changes, ensuring proper initialization/timer handling during deep sleep or wake events.
- **Power Plugin Integration:** Uses RDK's Power Manager Thunder plugin for system power state management and interaction.

### 2.3 Configuration Management

- **Dynamic Configuration:** Reads, updates, and applies runtime configuration changes from files, including support for vendor override files.
- **Device Discovery:** Provides mechanisms to discover IR input devices, support for multiple device types and fallback/stub implementations if hardware is absent.
- **HAL Abstraction:** Exposes interfaces to control hardware-specific features via C++ classes (see `ctrlm_hal.h`, `ctrlm_hal_rf4ce.h`, `ctrlm_hal_ble.h`).

### 2.4 Audio & Voice Control

- **Voice Session Management:** Supports BLE/RF voice streaming; logs sessions; manages session state, end times, and error reporting.
- **Audio Stream Management:** Reports and optimizes audio pipe size; ensures reliable audio sample reporting for voice sessions.

### 2.5 API & Service Integration

- **Thunder & HDMI Plugins:** Integrates with plugin frameworks to extend support for HDMI input, AV input, MAC address fetch, and advanced service bridging.
- **ASB Detection:** Offers runtime detection for Advanced Service Bridge capabilities.

## 3. Product Extensibility

- **Vendor Layer Integration:** Provides hooks for vendor-specific features, such as configuration overrides and device database stubs.
- **Flexible Build & Runtime Flags:** Build flags allow enabling/disabling features (e.g., BLE audio, packet analysis, deepsleep, memory lock).
- **Plugin & Target Customization:** Can build custom targets (e.g., just the control config file).

## 4. Typical Use Cases

- **User Experience:** Enables seamless remote pairing and input handling, responsive device wake/sleep, and dynamic feature provisioning.
- **Monitoring/Diagnostics:** Logs device events and telemetry, aiding both advanced diagnostics and data-driven product improvement.
- **Integration Point:** Forms the backbone for device control in RDK deployments where remote management and hardware abstraction are required.

## 5. Recent Product Updates (Selected Highlights)

- Multi-protocol remote integration (BLE and RF4CE support).
- Overhaul of key mapping and vendor integration logic.
- Telemetry improvements for voice stream analytics.
- Refactoring to enhance HAL interface extensibility.
- Crash and stability fixes for deep sleep, rapid input, and device discovery.
- Improved runtime configuration and plugin extensibility.

_For full release notes, please see the [CHANGELOG](https://github.com/rdkcentral/control/blob/develop/CHANGELOG.md)._

## 6. Summary

`rdkcentral/control` delivers a comprehensive control and management solution for RDK devices, emphasizing extensibility, reliability, and integration with modern remote protocols and power management frameworks. It remains the canonical product for remote control, device state, and configuration management within the RDK platform.
