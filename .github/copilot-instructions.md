# Copilot Instructions

## Repository overview
This is the **ctrlm-main** (Control Manager) plugin — a Thunder/WPEFramework plugin for RDK remote control management.

## ci/ directory
The `ci/` directory contains **native CI build support files only**. It is not part of the application.

- `ci/build_dependencies.sh` / `ci/cov_build.sh` — scripts that build the plugin in a CI container without a full RDK target image
- `ci/mocks/xlog_ci_compat.h` — minimal shim that pulls `std::string`, `std::map`, `std::tuple`, `std::get` into the global namespace (mirrors the transitive effect of the real rdkx_logger.h in the Yocto build)
- `ci/mocks/testframework_overrides.h` — supplements testframework mocks with declarations ctrlm needs that are not yet upstream
- `ci/mocks/devicesettings_ctrlm.patch` — patch applied to the testframework `devicesettings.h` at CI time for ctrlm-specific additions (ducking types, `setAudioDucking`, `Manager::IsInitialized`). Remove once these land in testframework develop.
- `ci/mocks/safec_lib.h` — compatibility shim mapping `safec_lib.h` to system libsafec headers
- `ci/headers/` — empty stub headers and real xr-voice-sdk headers generated/copied at CI build time; not committed to source

Real xr-voice-sdk headers (including `rdkx_logger.h`, `xr_voice_sdk.h`, and generated `rdkx_logger_modules.h`) are produced by `build_dependencies.sh` and placed in `ci/headers/xr-voice-sdk/`. Mock/stub headers for platform libraries (IARM, DeviceSettings, RFC, etc.) are sourced from `entservices-testframework/` at CI build time.

When suggesting code or answering questions, treat CI mocks as scaffolding, not as authoritative API definitions. For real API shapes refer to the installed headers under `install/usr/include/` or the upstream repositories (Thunder, entservices-apis, xr-voice-sdk).
