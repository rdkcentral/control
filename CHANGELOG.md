# Changelog

All notable changes to this project will be documented in this file.

* Each RDK Service has a CHANGELOG file that contains all changes done so far. When version is updated, add a entry in the CHANGELOG.md at the top with user friendly information on what was changed with the new version. Please don't mention JIRA tickets in CHANGELOG. 

* Please Add entry in the CHANGELOG for each version change and indicate the type of change with these labels:
    * **Added** for new features.
    * **Changed** for changes in existing functionality.
    * **Deprecated** for soon-to-be removed features.
    * **Removed** for now removed features.
    * **Fixed** for any bug fixes.
    * **Security** in case of vulnerabilities.

* Changes in CHANGELOG should be updated when commits are added to the main or release branches. There should be one CHANGELOG entry per JIRA Ticket. This is not enforced on sprint branches since there could be multiple changes for the same JIRA ticket during development. 

* In the future, generate this file by [`auto-changelog`](https://github.com/CookPete/auto-changelog).


## [1.0.10] - 2025-05-16

### Changed
- ctrlm crash from main queue msgs getting to networks prior to initialization
- ctrlm support both old and new deviceType
- Control manager crashes on rapid MIC button presses
- [Logging] Add Invalid firmware version OTA error code (0x8) mapping in ctrlmgr logs
- Changed deviceType from tv to IpTv.
- Extend adpcm frame info.


## [1.0.9] - 2025-04-28

### Changed
- Audio samples is always reported as 0 for FFV sessions
- Remove 'volatile' type qualifier of "binding_in_progress"
- Upgrading the halif-headers as iarmmgr


## [1.0.8] - 2025-04-15

### Changed
- use list of all possible names for IR input device discovery instead of product specific config


## [1.0.7] - 2025-03-31

### Changed
- Rationalize Voice Logging
- move auth from ctrlm-cpc to ctrlm-main
- Add ctrlm HAL certificate interace
- Remove irMgr dependencies in controlMgr
- create HAL interface for platform specific IRDBs

### Added
- BLE audio stream end time telemetry
- voice stream telemetry in single line/event

### Fixed
- Type-Z OTA bug
- Remote type changed to type-Z early
- controlMgr maintenance time crash at onInitializedTimer
- fix ctrlm_config.json.template issue
- Unable to start new voice stream with conversational UI server
- check that a file descriptor is valid before FD_SET()
- add correct ds-hal include path
- Logline error event rcu firmware status


## [1.0.6] - 2025-03-18

### Added
- custom target to build ctrlm config file only
- additional config override file that can be provided by vendor layer


## [1.0.5] - 2025-02-24

### Changed
- crash at onInitializedTimer when going to deepsleep


## [1.0.4] - 2025-02-20

### Changed
- removed references to deprecated irMgr component


## [1.0.3] - 2025-02-07

### Changed
- check that a file descriptor is valid before FD_SET()
- standardize use of singleton pattern ctrlm
- speed up BLE auto pairing and surface failures immediately
- move IR device input name to runtime config file
- Make writeAdvertisingConfig synchronous
- Remove legacy ipcontrol certs
- remove legacy url_vrex config field
- Add ctrlm Support for XRA BLE key - QAM
- ControlMgr crash pairWithMacHash when going to deepsleep

### Added
- RemoteControl plugin methods to pair and unpair targetted RCU devices based on MAC
- RemoteControl plugin methods to trigger RCU firmware upgrade and report status of upgrade
- ctrlm-factory added to this repo, its no longer a separate repo


## [1.0.2] - 2024-12-06

### Changed
- ctrlm IR uinput device match exact name, simplify IR-initiated BLE pairing event handling
- Check for Invalid avDevType
- move stop audio stream to separate non iarm related function
- Add Support for BLE keys - Accessibility, Guide, Info
- ctrlm crash in BLE adapter proxy during shutdown
- Detect the platform type (TV vs STB) using DeviceInfo plugin
- IR keypresses use same PII mask variable as Voice
- fix "last wakeup key code" not received, along with defering gdbus proxy calls for characteristics until they are needed.

### Added
- unit test function to set IR protocol support characteristic on RCU
- Added Alexa voice service support in SDT endpoint, along with async voice message support
