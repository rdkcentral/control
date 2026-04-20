# Copilot Instructions

## Repository overview
This is the **ctrlm-main** (Control Manager) plugin — a Thunder/WPEFramework plugin for RDK remote control management.

## ci/ directory
The `ci/` directory contains **native CI build support files only**. It is not part of the application.

- `ci/build_dependencies.sh` / `ci/cov_build.sh` — scripts that build the plugin in a CI container without a full RDK target image
- `ci/mocks/control/` — control-specific stub headers (rdkx_logger, xr_voice_sdk, rdkversion) that are not available upstream
- `ci/mocks/testframework_overrides.h` — supplements testframework mocks with declarations ctrlm needs that are not yet upstream
- `ci/headers/` — empty stub headers and real xr-voice-sdk headers generated/copied at CI build time; not committed to source

Mock/stub headers for platform libraries (IARM, DeviceSettings, RFC, Telemetry, Thunder, etc.) and Thunder/ThunderTools patches are sourced from `entservices-testframework/` at CI build time. Control-specific mocks remain in `ci/mocks/control/`. Real xr-voice-sdk headers are copied into `ci/headers/xr-voice-sdk/` and placed first on the include path to override testframework mocks where the real API matches.

When suggesting code or answering questions, treat CI mocks as scaffolding, not as authoritative API definitions. For real API shapes refer to the installed headers under `install/usr/include/` or the upstream repositories (Thunder, entservices-apis, xr-voice-sdk).
