# Carabiner

<image align="right" width="150" src="doc/assets/carabiner.jpg">

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

If you are on a Mac OS X, Windows x64, or Raspberry Pi system,
you can download the executable from the
[releases](https://github.com/brunchboy/carabiner/releases) page.
Then just open a terminal window, and run it:

    > Carabiner
    Starting Carabiner 1.1.2 on port tcp://127.0.0.1:17000
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

> If you want to run Carabiner as a system daemon (background process
> that starts when the system boots), add `--daemon` to the command line
> you configure to start it up, so that it does not spam the system log
> with status line updates that you can't see anyway.

## Clients

* [link-to-py](https://github.com/bdyetton/link-to-py) is a Python 3
  module for interacting with Carabiner written
  by [Benjamin Yetton](https://github.com/bdyetton).

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
repository. On the Mac, and other Unix platforms:

    mkdir build
    cd build
    cmake ..
    cmake --build .

Once the build completes you will find the executable in
`bin/Carabiner` under the `build` directory.

> :wrench: On Windows, you need to install both cmake for Windows and
> Visual Studio 2017. Then you can use the following variation
> (after updating the git submodules, as described above):

    mkdir build
    cd build
    cmake -g "Visual Studio 15 2017" ..
    cmake --build . --config Release

Once the build completes you will find the executable in
`bin\Release\Carabiner.exe` under the `build` directory.

## Protocol

Client programs interact with Carabiner by sending and receiving
simple single-packet messages.

> :point_right: To protect against multiple messages being read as
> one, because they were sent right after one another and delivered as
> a single network packet, you should always put a newline character
> at the end of the message. As of version 1.1.0, Carabiner can
> process multiple commands in a single network packet, as long as
> they are separated by newlines. Trailing newlines will not cause any
> problems for older versions of Carabiner, but those versions will
> not see any commands following the first in a given packet.
>
> Similarly, starting with version 1.1.0, Carabiner places newline
> characters at the end of its own responses, and clients should be
> prepared to deal with multiple responses in a single network packet
> in case they have sent a query at roughly the same time that there
> was a change in the Link status to be reported on.

The currently supported messages are:

### status

Sending the string `status` to Carabiner requests an update containing
the current status. It will respond with a packet like the following:

    status { :peers 0 :bpm 120.000000 :start 73743731220 :beat 597.737570 }

As with all non-error messages from Carabiner, this consists of a
message type identifier followed by an
[edn](https://github.com/edn-format/edn#edn) structure containing the
message parameters. In this case, it identifies the current number of
Link peers, the current tempo of the Link session, the timestamp
of when the session timeline started, and the current beat position.

If you have enabled Start/Stop Sync, status responses will also
include the current transport state:

    status { :peers 0 :bpm 120.000000 :start 73743731220 :beat 597.737570 :playing true }

A status response will also be sent when you first connect to
Carabiner, and whenever the Link session tempo changes, as well as
whenever the number of Link peers changes, and (if you have enabled
Start/Stop Sync) when the transport state changes.

> :stopwatch: **About Link Timestamps:** The `:start` value in the
> above message, and the time and `:when` values sent and returned in
> the commands below, are expressed in microseconds, but they are
> *not* interchangeable with values returned by the normal "wall
> clock" system clock you use to determine the current time of day.
> Link needs to use a monotonically increasing clock that is not
> affected by things like leap seconds or NTP server synchronization.
> In Java you can obtain Link-compatible timestamps using the
> `System.nanoTime()` method. If you are working in other languages,
> you will need to experiment in order to find out how to read the
> same clock that Link is using.

### bpm

Sending the string `bpm ` followed by a floating-point value, for
example `bpm 140.0`, tells Carabiner to immediately set the Link
session's tempo to the specified value. If this is a change from the
previous value, it will result in a `status` response as described
above.

If the bpm value is missing, cannot be parsed as a floating point
number, or is outside the range from 20.0 to 999.0, the session tempo
is left unchanged and Carabiner responds with the message `bad-bpm `.

> If your client wants to use a tempo outside the range supported by
> Link, Ableton recommends setting the Link tempo to the closest
> multiple or fraction which is in range. For example, if the user
> wants 15 BPM, set the session tempo to 30 BPM.

### beat-at-time

Sending the string `beat-at-time ` followed by a microsecond timestamp
(an integer relative to the `:start` value returned in the `status`
response), and a quantum value (which identifies the number of beats
that make up a bar or loop as described in the Phase Synchronization
section of the [Link documentation](http://ableton.github.io/link/)),
asks Carabiner to identify the beat which falls at the specified point
on the Link session timeline. For example, sending `beat-at-time
73746356220 4` to the link session used in the above examples would
result in a response like the following:

    beat-at-time { :when 73746356220 :quantum 4.000000 :beat 5.250000 }

This response indicates that at the specified microsecond along the
Link session timeline, we are a quarter of the way from the sixth to
the seventh beat (Link numbers beats starting with 0).

If one of the parameters is missing or cannot be parsed, Carabiner
responds with `bad-time ` or `bad-quantum `.

### phase-at-time

Sending the string `phase-at-time ` followed by a microsecond
timestamp (an integer relative to the `:start` value returned in the
`status` response), and a quantum value (which identifies the number
of beats that make up a bar or loop as described in the Phase
Synchronization section of
the [Link documentation](http://ableton.github.io/link/)), asks
Carabiner to identify the phase which falls at the specified point on
the Link session timeline. A phase is a floating point value ranging
fom 0.0 to just less than the quantum. For example, sending
`phase-at-time 73746356220 4` to the link session used in the above
examples would result in a response like the following:

    phase-at-time { :when 73746356220 :quantum 4.000000 :phase 1.250000 }

This response indicates that at the specified microsecond along the
Link session timeline, we are just over a quarter of the way from the
second to the third beat of the four beats in a quantum period.

If one of the parameters is missing or cannot be parsed, Carabiner
responds with `bad-time ` or `bad-quantum `.

### time-at-beat

Sending the string `time-at-beat ` followed by a beat number (a
floating point value, since you can inquire about points that fall
between beats), and a quantum value (which identifies the number of
beats that make up a bar or loop as described in the Phase
Synchronization section of
the [Link documentation](http://ableton.github.io/link/)), asks
Carabiner to return the microsecond timestamp (an integer relative to
the `:start` value returned in the `status` response) at which the
specified beat (or fraction of a beat) falls on the Link session
timeline. For example, sending `time-at-beat 100 4` to the link
session used in the above examples would result in a response like the
following:

    time-at-beat { :beat 100.000000 :quantum 4.000000 :when 73793731220 }

This response indicates that the hundred and first beat falls at the
specified microsecond within the timeline. As a sanity check, you can
ask about the first beat, and verify that the `:when` value matches
the `:start` value reported by the `status` message. Sending
`time-at-beat 0 4` in the session used in these examples results in
the following response, which you can compare to the `status` response
above:

    time-at-beat { :beat 0.000000 :quantum 4.000000 :when 73743731220 }

Sending another `status` command now shows that the current beat has
moved on while these examples were being written, but the timeline
start point has remained the same:

    status { :peers 0 :bpm 120.000000 :start 73743731220 :beat 2560.623742 }

If one of the parameters is missing or cannot be parsed, Carabiner
responds with `bad-beat ` or `bad-quantum `.

### force-beat-at-time

Sending the string `force-beat-at-time ` followed by a floating-point
beat number, a microsecond timestamp (an integer relative to the
`:start` value returned in the `status` response), and a quantum value
(as described above) tells Carabiner to forcibly and abruptly adjust
the Link session timeline so that the specified beat falls at the
specified point in time. The change will be communicated to all
participants, and will result in audible shifts in playback.

Continuing the previous example, sending `force-beat-at-time 1.0
73746356220 4` will tell Carabiner to adjust the Link session timeline
so the second beat starts as close as possible to the specified moment
(which previously was 25% of the way from the sixth to the seventh
beat). Carabiner responds with a `status` message which reports the
new `:start` timestamp of the timeline.

    status { :peers 0 :bpm 120.000000 :start 73745856220 :beat 2989.161370 }

At this point, repeating the `beat-at-time` command we used in the
section explaining that command, `beat-at-time 73746356220 4`, will
return a beat value that is very close to 1.0 (in this example it was
exact):

    beat-at-time { :when 73746356220 :quantum 4.000000 :beat 1.000000 }

If one of the parameters is missing or cannot be parsed, Carabiner
responds with `bad-beat `, `bad-time `, or `bad-quantum `.

This command should only be used when synchronizing a Link session to
an external timing source which cannot participate in the
consensus-based Link timeline, and should be done only when the
session has drifted beyond some reasonable threshold, so that external
jitter does not lead to excessive adjustments.

> If you are building an application that can perform quantized
> starts, and thereby participate in a Link session more graciously,
> without requiring the other participants to shift the timeline, you
> should use the following command instead:

### request-beat-at-time

Sending the string `request-beat-at-time ` followed by a
floating-point beat number, a microsecond timestamp (an integer
relative to the `:start` value returned in the `status` response), and
a quantum value (as described above) tells Carabiner to ask Link to
try to gracefully adjust its timeline so that the specified beat will
occur at the specified time. If there are no other peers in the Link
session, this will behave the same as `force-beat-at-time`, above.
However, if there are any peers, it will avoid the kinds of audible
discontinuities described above, by adjusting the local timeline so
that the specified beat will instead fall at the next point in time
*after* the requested time which has the same *phase* as the specified
beat.

Carabiner responds with a `status` message which reports the
new `:start` timestamp of the timeline.

If one of the parameters is missing or cannot be parsed, Carabiner
responds with `bad-beat `, `bad-time `, or `bad-quantum `.

As the Link documentation explains, this command is specifically
designed to enable the concept of "quantized launch". If there are no
peers, the deisred mapping is established immediately when requested.
Otherwise, we wait until the next time at which the session phase
matches the desired event, so we can seamlessly join the peers that
are already in the session.

### enable-start-stop-sync

Sending the string `enable-start-stop-sync` tells Carabiner to start
synchronizing transport (playing) state with peers on the Link
network. This is only supported in Link version 3 and later (which is
used in Ableton Live 10 and later). Older peers will neither report
their transport state, nor respond to changes you send through
Carabiner.

Carabiner responds with a `status` message which now includes a
`:playing` value that reports the current transport state, `true` or
`false`. From this point on, any changes to the shared Link transport
state will cause a new `status` message to be sent to report the new
state.

### disable-start-stop-sync

Sending the string `disable-start-stop-sync` tells Carabiner to stop
synchronizing transport (playing) state with peers on the Link
network.

Carabiner responds with a `status` message which no longer includes a
`:playing` value, and will no longer send status updates when the Link
transport status changes.

### start-playing

Sending the string `start-playing` followed by a microsecond timestamp
(an integer relative to the `:start` value returned in the `status`
response) when Start/Stop Sync is enabled tells Carabiner to set the
Link transport state to "playing", and inform any peers that are also
participating in Start/Stop Sync.

Carabiner responds with a `status` message which reflects the new
transport state.

### stop-playing

Sending the string `stop-playing` followed by a microsecond timestamp
(an integer relative to the `:start` value returned in the `status`
response) when Start/Stop Sync is enabled tells Carabiner to set the
Link transport state to "stopped", and inform any peers that are also
participating in Start/Stop Sync.

Carabiner responds with a `status` message which reflects the new
transport state.

### version

Sending the string `version` asks Carabiner to report its version
number. This is only supported starting with verson 1.1.0; previous
versions will respond `unsupported version` (which sounds pretty
on-point, but really means that they don't know about the `version`
command), so you can assume that they are version 1.0.0 or earlier.
Supported versions respond with `version` followed by the version
string, for example `version "1.1.0"`.


## Apology

I am not a C++ programmer; the last time I wrote anything in it before
Carabiner was in the early 1990s. This seems to work, but I am sure it
could be written much better. Suggestions for improvement are
definitely welcome!

## Licenses

<img align="right" alt="Deep Symmetry" src="doc/assets/DS-logo-bw-200-padded-left.png">

Carabiner is Copyright © 2016-2019 [Deep Symmetry, LLC](http://deepsymmetry.org)

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
