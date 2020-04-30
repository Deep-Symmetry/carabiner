# Change Log

All notable changes to this project will be documented in this file.
This change log follows the conventions of
[keepachangelog.com](http://keepachangelog.com/).

## [Unreleased][unreleased]

### Changed

- The macOS build process now creates HFS+ (MacOS Extended) disk
  images rather than the default APFS filesystem that it was
  previously using. This allows them to be opened on High Sierra
  (10.12) and earlier.

## [1.1.3] - 2020-01-16

### Added

- Now automatically build and release the Mac and Windows executables
  whenever changes are pushed to this repository, thanks to the power
  of GitHub Actions.
- Mac executables are released as code-signed, notarized disk images
  for ease of use on Catalina.

### Changed

- Updated to Link 3.0.2 and gflags 2.2.2.

## [1.1.2] - 2019-12-17

### Added

- A `--daemon` boolean option to shut off the status line output,
  since that does nothing but flood the system log when Carabiner is
  running as an operating system background process.
- A Raspberry Pi build, since I now own one.
- Also now reports the version number in the startup message, which is
  helpful when scanning the system log.

## [1.1.1] - 2019-04-24

### Fixed

- It turns out that C++ `long` types [in 64-bit
  Windows](https://en.wikipedia.org/wiki/64-bit_computing#64-bit_data_models)
  are half the size of the Java `long` type used by Beat Link Trigger,
  so values BLT is sending as time stamps need to be declared as `long
  long` values in C++ to be properly received in Windows. This change
  fixes parse errors people were running into. (It should also make it
  safe to compile and run in a 32-bit OS.)

### Added

- More detailed error information is printed in the window in which
  Carabiner is running when there is a problem parsing a value it has
  received. Hopefully this can help figure out a problem people are
  running into in Windows sometimes when the beat numbers for the
  `beat-at-time` command are being unusable.

## [1.1.0] - 2018-08-27

### Added

- A way for clients to check the Carabiner version, so Beat Link
  Trigger can know whether the multi-message packets issue has been
  fixed and warn the user to upgrade Carabiner if needed.

### Fixed

- If multiple messages were sent rapidly to or from Carabiner they
  might get grouped into a single network packet, and the later ones
  would be ignored. This release, along with a newer Beat Link Trigger
  release, process even later messages grouped in the same packet.


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

[unreleased]: https://github.com/Deep-Symmetry/carabiner/compare/v1.1.3...HEAD
[1.1.3]: https://github.com/Deep-Symmetry/carabiner/compare/v1.1.2...v1.1.3
[1.1.2]: https://github.com/Deep-Symmetry/carabiner/compare/v1.1.1...v1.1.2
[1.1.1]: https://github.com/Deep-Symmetry/carabiner/compare/v1.1.0...v1.1.1
[1.1.0]: https://github.com/Deep-Symmetry/carabiner/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/Deep-Symmetry/carabiner/compare/v0.1.3...v1.0.0
[0.1.3]: https://github.com/Deep-Symmetry/carabiner/compare/v0.1.2...v0.1.3
[0.1.2]: https://github.com/Deep-Symmetry/carabiner/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/Deep-Symmetry/carabiner/compare/v0.1.0...v0.1.1
