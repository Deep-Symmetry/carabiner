# Carabiner

<image align="right" width="300" src="doc/assets/carabiner.jpg">

A loose connector for interacting with
[Ableton Link](https://www.ableton.com/en/link/).

Carabiner is a program that embeds the C++
[Link library](https://github.com/Ableton/link) and listens for local
TCP connections to allow other programs, like
[beat-link-trigger](https://github.com/brunchboy/beat-link-trigger#beat-link-trigger)
and [Afterglow](https://github.com/brunchboy/afterglow#afterglow), to
participate in some aspects of a Link session, even though they were
not written using C++ compatible languages and runtimes.

> Carabiner does not support the full power and elegance of Link, so
> if you are writing a program that can use the library directly, you
> are much better off doing that instead. Carabiner is for cases where
> that is impossible or impractical.

## Usage

If you are on a Mac OS X system, you can download the executable from
the [releases](releases) page. Then just open a terminal window, and
run it:

    > Carabiner
    Starting Carabiner on port tcp://127.0.0.1:17000
    Link bpm: 128.5 Peers: 1 Connections: 0

By default it listens on port 17000. If this port is already in use on
your system, you can specify a different port using the `--port`
command-line argument.

The status line shows the current tempo of the Link session, and the
number of Link Peers which have been found. It also shows how many TCP
connections are currently open to Carabiner from client programs like
beat-link-trigger. See the documentation of the client program for
instructions on how to have it connect to Carabiner.

You can shut down Carabiner by typing `Control-C` in the Terminal
window, or simply closing that window.

## Building

If you want to run Carabiner on a different platform, or make and test
modifications to its source code, you can build it yourself.

Carabiner relies on `link` and `gflags` as `git` submodules (and link
relies on its own submodules), so after you have cloned the main
`carabiner` repository, you need to `cd` into it and set up the
submodules by running:

    git submodule update --init --recursive

Carabiner uses [CMake](https://www.cmake.org/) to manage its build
process. Once again, from the top-level directory of your `carabiner`
repository:

    mkdir build
    cd build
    cmake ..
    cmake --build .

Once the build completes you will find the executable in
`bin/Carabiner` under the `build` directory.

## Protocol

Client programs interact with Carabiner by sending and receiving
simple single-packet messages. The currently supported messages are:

### status

Sending the string `status` to Carabiner requests an update containing
the current status. It will respond with a packet like the following:

    status { :peers 1 :bpm 128.500019 :start 128928334480 }

As with all non-error messages from Carabiner, this consists of a
message type identifier followed by an
[edn](https://github.com/edn-format/edn#edn) structure containing the
message parameters. In this case, it identifies the current number of
Link peers, the current tempo of the Link session, and the timestamp
of when the session timeline started.

A status response will also be sent when you first connect to
Carabiner, and whenever the Link session tempo changes, as well as
whenever the number of Link peers changes.

### bpm

Sending the string `bpm ` followed by a floating-point value, for
example `bpm 140.0`, tells Carabiner to immediately change the Link
session's tempo to the specified value. If this is a change from the
previous value, it will result in a `status` response as described
above.

If the bpm value is missing, cannot be parsed as a floating point
number, or is outside the range from 0.9 to 400.0, the session tempo
is left unchanged and Carabiner responds with the message `bad-bpm `
followed by the argument you supplied.

### beat-at-time

Sending the string `beat-at-time ` followed by a nanosecond timestamp
(an integer relative to the `:start` value returned in the `status`
response), and a quantum value (which identifies the number of beats
that make up a bar or loop as described in the Phase Synchronization
section of the [Link documentation](http://ableton.github.io/link/)),
asks Carabiner to identify the beat which falls at the specified point
on the Link session timeline. For example, sending `beat-at-time
129426900000 4` to the link session used in the above examples
would result in a response like the following:

    beat-at-time { :when 129426900000 :quantum 4.000000 :beat 1.278066 }

This response indicates that at the specified nanosecond along the
Link session timeline, we are just over a quarter of the way from the
second to the third beat.

If one of the parameters is missing or cannot be parsed, Carabiner
responds with `bad-time ` or `bad-quantum ` followed by the arguments
you gave it.

### force-beat-at-time

Sending the string `force-beat-at-time ` followed by a floating-point
beat number, a nanosecond timestamp (an integer relative to the
`:start` value returned in the `status` response), and a quantum value
(as described above) tells Carabiner to forcibly and abruptly adjust
the Link session timeline so that the specified beat falls at the
specified point in time. The change will be communicated to all
participants, and will result in audible shifts in playback.

Continuing the previous example, sending `force-beat-at-time 1.0
129426900000 4` will tell Carabiner to adjust the Link session
timeline so the second beat starts as close as possible to the
specified moment (which previously was 28% of the way to the third
beat). Carabiner responds with a `status` message which reports the
new `:start` timestamp of the timeline.

    status { :peers 1 :bpm 140.000000 :start 129426471429 }

At this point, repeating the `beat-at-time` command from the previous
section will return a beat value that is very close to 1.0.

    beat-at-time { :when 129426900000 :quantum 4.000000 :beat 0.999984 }

If one of the parameters is missing or cannot be parsed, Carabiner
responds with `bad-beat `, `bad-time `, or `bad-quantum ` followed by
the arguments you gave it.

This command should only be used when synchronizing a Link session to
an external timing source which cannot participate in the
consensus-based Link timeline, and should be done only when the
session has drifted beyond some reasonable threshold, so that external
jitter does not lead to excessive adjustments.

> If you are building an application that can perform quantized
> starts, and thereby participate in a Link session more graciously,
> without requiring the other participants to shift the timeline, then
> open an [issue](issues) to have the `request-beat-at-time` command
> implemented by Carabiner.

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

[gflags](https://github.com/gflags/gflags) is Copyright © 2006, Google
Inc. It has its own [license](GFLAGS_LICENSE.txt).
