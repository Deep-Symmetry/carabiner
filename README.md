# carabiner

A loose connector for interacting with Ableton Link

This project is not ready for general use; it is on GitHub to
facilitate Ableton's gracious assistance in getting the CMake build
process fixed.

## Building

Carabiner relies on `link` as a `git` submodule (and that relies on
its own submodules), so after you have cloned the main `carabiner`
repository, you need to `cd` into it and set up the submodules by
running:

    git submodule update --init --recursive

Carabiner uses [CMake](https://www.cmake.org/) to manage its build
process. Once again, from the top-level directory of your `carabiner`
repository:

    mkdir build
    cd build
    cmake ..
    cmake --build .

## Licenses

<img align="right" alt="Deep Symmetry" src="doc/assets/DS-logo-bw-200-padded-left.png">
Carabiner is Copyright © 2016 [Deep Symmetry, LLC](http://deepsymmetry.org)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either
[version 2 of the License](LICENSE.md), or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see
[http://www.gnu.org/licenses/](http://www.gnu.org/licenses/).

[Link](https://github.com/Ableton/link) and
[Mongoose](https://github.com/cesanta/mongoose) are dual-licensed.
While available through the same GPL v2 as Carabiner itself, they are
also available through proprietary licenses for embedding in non-free
software, by contacting their respective developers.
