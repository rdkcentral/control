# Copilot Instructions

## Repository overview
This is the **ctrlm-main** (Control Manager) plugin — a Thunder/WPEFramework plugin for RDK remote control management.

## ci/ directory
The `ci/` directory contains **native CI build support files only**. It is not part of the application.

- `ci/build_dependencies.sh` / `ci/cov_build.sh` — scripts that build the plugin in a CI container without a full RDK target image
- `ci/mocks/` — minimal stub headers that stand in for platform libraries (IARM, DeviceSettings, RFC, Telemetry, xr-voice-sdk, etc.) that are unavailable in the CI environment. These are **not** production implementations.
- `ci/patches/` — temporary patches applied to Thunder/ThunderTools during CI builds only
- `ci/headers/` — empty stub headers generated at CI build time; not committed to source

When suggesting code or answering questions, treat `ci/mocks/` as CI scaffolding, not as authoritative API definitions. For real API shapes refer to the installed headers under `install/usr/include/` or the upstream repositories (Thunder, entservices-apis, xr-voice-sdk).
