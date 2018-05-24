# Change Log

All notable changes to this project will be documented in this file.
This change log follows the conventions of
[keepachangelog.com](http://keepachangelog.com/).

## [Unreleased][unreleased]

Nothing so far.

## [1.0.0] - 2018-05-24

### Added

- Implemented the `enable-start-stop-sync`, `disable-start-stop-sync`,
  `start-playing`, and `stop-playing` commands, to support the new
  transport control features in Link 3.0.
- A Windows binary, and a tip linking to instructions for building it
  in that environment.

### Changed

- Updated the embedded Link library to version 3.0.1 to be able to
  support start/stop sync.

## [0.1.3] - 2017-07-23

### Added

- Implemented the `phase-at-time`, `time-at-beat`, and `request-beat-at-time`
  commands, to provide access to the rest of the Link API.
- Explained the nature of Link timestamps in the documentation.

### Changed

- Improved some details in the documentation.
- Updated the embedded Link library to incorporate fixes.

## [0.1.2] - 2017-04-22

### Changed

- Updated the embedded Link library to incorporate recent
  improvements, such as supporting Carabiner on the Raspberry Pi.

## [0.1.1] - 2016-11-20

### Added

- Beat position information in status updates, for Afterglow.

### Fixed

- Support the actual tempo range that Link itself supports, from 20 to
  999 BPM.

## 0.1.0 - 2016-11-09

### Added

- Set up initial project structure.
- Figured out how to embed and work with the Link library.
- Chose communication and configuration frameworks.
- Built an implementation which meets the needs of beat-link-trigger.

[unreleased]: https://github.com/brunchboy/carabiner/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/brunchboy/carabiner/compare/v0.1.3...v1.0.0
[0.1.3]: https://github.com/brunchboy/carabiner/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/brunchboy/carabiner/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/brunchboy/carabiner/compare/v0.1.0...v0.1.1
