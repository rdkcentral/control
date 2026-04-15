# Copilot Instructions

## Repository overview
This is the **ctrlm-main** (Control Manager) plugin — a Thunder/WPEFramework plugin for RDK remote control management.

## ci/ directory
The `ci/` directory contains **native CI build support files only**. It is not part of the application.

- `ci/build_dependencies.sh` / `ci/cov_build.sh` — scripts that build the plugin in a CI container without a full RDK target image
- `ci/patches/` — temporary patches applied to Thunder/ThunderTools during CI builds only
- `ci/headers/` — empty stub headers and real xr-voice-sdk headers generated/copied at CI build time; not committed to source

Mock/stub headers for platform libraries (IARM, DeviceSettings, RFC, Telemetry, etc.) are sourced from `entservices-testframework/Tests/mocks/` at CI build time, not stored in this repo. Real xr-voice-sdk headers are copied into `ci/headers/xr-voice-sdk/` and placed first on the include path to override those mocks.

When suggesting code or answering questions, treat CI mocks as scaffolding, not as authoritative API definitions. For real API shapes refer to the installed headers under `install/usr/include/` or the upstream repositories (Thunder, entservices-apis, xr-voice-sdk).
