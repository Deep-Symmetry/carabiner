#include <string>
#include <set>
#include <mutex>

#include <gflags/gflags.h>

#include <ableton/Link.hpp>

extern "C" {
#include "mongoose.h"
}

// The version number, for the command line, as well as the client query.
static const std::string version = "1.1.0";

// Validators for command-line arguments
static bool validatePort(const char* flagname, gflags::int32 value) {
  if (value > 0 && value < 32768) {
    return true;  // Valid
  }
  std::cerr << flagname << " must be between 1 and 32767" << std::endl;
  return false;
}

static bool validatePoll(const char* flagname, gflags::int32 value) {
  if (value > 0 && value < 1001) {
    return true;  // Valid
  }
  std::cerr << flagname << " must be between 1 and 1000" << std::endl;
  return false;
}

// Set up the command-line options supported by the program
DEFINE_int32(port, 17000, "TCP port on which to accept connections");
static const bool port_dummy = gflags::RegisterFlagValidator(&FLAGS_port, &validatePort);
DEFINE_int32(poll, 20, "Number of milliseconds between updates");
static const bool poll_dummy = gflags::RegisterFlagValidator(&FLAGS_poll, &validatePoll);

// Our interface to the Link session
ableton::Link linkInstance(120.);

// Keep track of open TCP connections, and those which need updates sent to them
std::set<struct mg_connection *> activeConnections, updatedConnections;
std::mutex updatedMutex;

// Keep track of whether we are synchronizing transport start/stop
bool syncStartStop = false;

// Send a message describing the current state of the Link session.
static void reportStatus(struct mg_connection *nc) {
  const std::chrono::microseconds time = linkInstance.clock().micros();
  const ableton::Link::SessionState sessionState = linkInstance.captureAppSessionState();
  std::string playingState = sessionState.isPlaying()? "true" : "false";
  std::string playingResponse = syncStartStop? (" :playing " + playingState) : "";
  std::string response = "status { :peers " + std::to_string(linkInstance.numPeers()) +
    " :bpm " + std::to_string(sessionState.tempo()) +
    " :start " + std::to_string(sessionState.timeAtBeat(0.0, 4.0).count()) +
    " :beat " + std::to_string(sessionState.beatAtTime(time, 4.0)) + playingResponse + " }\n";
  mg_send(nc, response.data(), response.length());
}

// Process a request for the current link session status. Can simply remove the connection
// from the set of those which have current status updates, and one will be sent on the next
// poll.
static void handleStatus(std::stringstream &args, struct mg_connection *nc) {
  updatedMutex.lock();
  updatedConnections.erase(nc);
  updatedMutex.unlock();
}

static void handleVersion(std::stringstream &args, struct mg_connection *nc) {
  std::string response = "version \"" + version + "\"\n";
  mg_send(nc, response.data(), response.length());
}

// Report that an argument has not been usable, and skip to the next command within the
// network packet, if any.
static void reportBadArgument(std::string code, std::string message, std::stringstream &args,
                              struct mg_connection *nc) {
  std::string response = code + '\n';
  mg_send(nc, response.data(), response.length());
  std::cerr << std::endl << message << std::endl;
  args.clear();
  args.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Process a request to establish a specific tempo
static void handleBpm(std::stringstream &args, struct mg_connection *nc) {
  double bpm;

  args >> bpm;
  if (args.fail() || (bpm < 20.0) || (bpm > 999.0)) {
    reportBadArgument("bad-bpm", "Failed to parse bpm", args, nc);
  }  else {
    ableton::Link::SessionState sessionState = linkInstance.captureAppSessionState();
    sessionState.setTempo(bpm, linkInstance.clock().micros());
    linkInstance.commitAppSessionState(sessionState);
    // No neeed to call reportStatus(nc) because if the BPM changed that will happen on its own.
  }
}

// Process a request to query the SessionState to find out what beat occurs at a particular time
static void handleBeatAtTime(std::stringstream &args, struct mg_connection *nc) {
  long when;
  double quantum;

  args >> when;
  if (args.fail()) {
    reportBadArgument("bad-time", "Failed to parse beat time", args, nc);
  } else {
    args >> quantum;
    if (args.fail() || (quantum < 2.0) || (quantum > 16.0)) {
      // Unparsed quantum value, report error
      reportBadArgument("bad-quantum", "Failed to parse beat quantum", args, nc);
    } else {
      ableton::Link::SessionState sessionState = linkInstance.captureAppSessionState();
      double beat = sessionState.beatAtTime(std::chrono::microseconds(when), quantum);
      std::string response = "beat-at-time { :when " + std::to_string(when) +
        " :quantum " + std::to_string(quantum) +
        " :beat " + std::to_string(beat) + " }\n";
      mg_send(nc, response.data(), response.length());
    }
  }
}

// Process a request to query the current SessionState phase
static void handlePhaseAtTime(std::stringstream &args, struct mg_connection *nc) {
  long when;
  double quantum;

  args >> when;
  if (args.fail()) {
    reportBadArgument("bad-time", "Failed to parse phase time", args, nc);
  } else {
    args >> quantum;
    if (args.fail() || (quantum < 2.0) || (quantum > 16.0)) {
      reportBadArgument("bad-quantum", "Failed to parse phase quantum", args, nc);
    } else {
      ableton::Link::SessionState sessionState = linkInstance.captureAppSessionState();
      double phase = sessionState.phaseAtTime(std::chrono::microseconds(when), quantum);
      std::string response = "phase-at-time { :when " + std::to_string(when) +
        " :quantum " + std::to_string(quantum) +
        " :phase " + std::to_string(phase) + " }\n";
      mg_send(nc, response.data(), response.length());
    }
  }
}

// Process a request to determine when a specific beat falls on the SessionState
static void handleTimeAtBeat(std::stringstream &args, struct mg_connection *nc) {
  double beat;
  double quantum;

  args >> beat;
  if (args.fail()) {
    reportBadArgument("bad-beat", "Failed to parse beat number", args, nc);
  } else {
    args >> quantum;
    if (args.fail() || (quantum < 2.0) || (quantum > 16.0)) {
      reportBadArgument("bad-quantum", "Failed to parse beat quantum", args, nc);
    } else {
      ableton::Link::SessionState sessionState = linkInstance.captureAppSessionState();
      std::chrono::microseconds time = sessionState.timeAtBeat(beat, quantum);
      auto micros = std::chrono::duration_cast<std::chrono::microseconds>(time);
      std::string response = "time-at-beat { :beat " + std::to_string(beat) +
        " :quantum " + std::to_string(quantum) +
        " :when " + std::to_string(micros.count()) + " }\n";
      mg_send(nc, response.data(), response.length());
    }
  }
}

// Process a request to gracefully or forcibly realign the SessionState
static void handleAdjustBeatAtTime(std::stringstream &args, struct mg_connection *nc, bool force) {
  double beat;
  long when;
  double quantum;

  args >> beat;
  if (args.fail()) {
    reportBadArgument("bad-beat", "Failed to parse beat number for adjustment", args, nc);
  } else {
    args >> when;
    if (args.fail()) {
      reportBadArgument("bad-time", "Failed to parse beat time for adjustment", args, nc);
    } else {
      args >> quantum;
      if (args.fail() || (quantum < 2.0) || (quantum > 16.0)) {
        reportBadArgument("bad-quantum", "Failed to parse beat quantum for adjustment", args, nc);
      } else {
        ableton::Link::SessionState sessionState = linkInstance.captureAppSessionState();
        if (force) {
          sessionState.forceBeatAtTime(beat, std::chrono::microseconds(when), quantum);
        } else {
          sessionState.requestBeatAtTime(beat, std::chrono::microseconds(when), quantum);
        }
        linkInstance.commitAppSessionState(sessionState);
        reportStatus(nc);
      }
    }
  }
}

// Process a request to enable or disable start/stop sync
static void handleEnableStartStopSync(bool enable, std::stringstream &args, struct mg_connection *nc) {
  syncStartStop = enable;
  linkInstance.enableStartStopSync(enable);
  reportStatus(nc);
}

// Process a request to start or stop the shared transport state
static void handleSetIsPlaying(bool playing, std::stringstream &args, struct mg_connection *nc) {
  long when;

  args >> when;
  if (args.fail()) {
    reportBadArgument("bad-time", "Failed to parse time for play or stop", args, nc);
  } else {
    ableton::Link::SessionState sessionState = linkInstance.captureAppSessionState();
    sessionState.setIsPlaying(playing, std::chrono::microseconds(when));
    linkInstance.commitAppSessionState(sessionState);
    reportStatus(nc);
  }
}

// When a packet has been received, identify the command it contains, and handle it appropriately.
static void processMessage(std::string msg, struct mg_connection *nc) {
  //std::cout << std::endl << "received: " << msg << std::endl;

  std::stringstream args(msg);
  args >> std::ws;
  while (args.peek() != EOF) {
    std::string cmd;
    args >> cmd;
    if (cmd == "bpm") {
      handleBpm(args, nc);
    } else if (cmd == "beat-at-time") {
      handleBeatAtTime(args, nc);
    } else if (cmd == "phase-at-time") {
      handlePhaseAtTime(args, nc);
    } else if (cmd == "time-at-beat") {
      handleTimeAtBeat(args, nc);
    } else if (cmd == "force-beat-at-time") {
      handleAdjustBeatAtTime(args, nc, true);
    } else if (cmd == "request-beat-at-time") {
      handleAdjustBeatAtTime(args, nc, false);
    } else if (cmd == "enable-start-stop-sync") {
      handleEnableStartStopSync(true, args, nc);
    } else if (cmd == "disable-start-stop-sync") {
      handleEnableStartStopSync(false, args, nc);
    } else if (cmd == "start-playing") {
      handleSetIsPlaying(true, args, nc);
    } else if (cmd == "stop-playing") {
      handleSetIsPlaying(false, args, nc);
    } else if (cmd == "status") {
      handleStatus(args, nc);
    } else if (cmd == "version") {
      handleVersion(args, nc);
    } else {
      // Unrecognized input, report error
      std::string response = "unsupported " + cmd + '\n';
      mg_send(nc, response.data(), response.length());
      std::cerr << std::endl << "Unrecognized command: " << cmd << std::endl;
      args.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    args >> std::ws;
  }
}

static void eventHandler(struct mg_connection *nc, int ev, void *p) {
  struct mbuf *io = &nc->recv_mbuf;
  (void) p;
  bool needsUpdate;

  switch (ev) {

  case MG_EV_ACCEPT:
    activeConnections.insert(nc);
    break;

  case MG_EV_CLOSE:
    activeConnections.erase(nc);
    break;

  case MG_EV_POLL:
    updatedMutex.lock();
    needsUpdate = updatedConnections.insert(nc).second;
    updatedMutex.unlock();
    if (needsUpdate) {
      // The connection was not already on the updated list, so send it an updated status
      reportStatus(nc);
    }
    break;

  case MG_EV_RECV:
    processMessage(std::string(io->buf, io->len), nc);
    mbuf_remove(io, io->len);       // Discard message from recv buffer
    break;
  default:
    break;
  }
}

// Registered to be called by a Link thread whenever the session BPM changes; arranges for an update to
// be sent to all of our TCP clients.
void tempoCallback(double bpm) {
  updatedMutex.lock();
  updatedConnections.clear();
  updatedMutex.unlock();
}

// Registered to be called by a Link thread whenever the number of peers changes; arranges for an update to
// be sent to all of our TCP clients.
void peersCallback(std::size_t numPeers) {
  updatedMutex.lock();
  updatedConnections.clear();
  updatedMutex.unlock();
}

// Registered to be called by a Link thread whenever the transport state changes if we are synchronizing
// start/stop state; arranges for an update to be sent to all of our TCP clients.
void startStopCallback(bool isPlaying) {
  updatedMutex.lock();
  updatedConnections.clear();
  updatedMutex.unlock();
}

int main(int argc, char* argv[]) {
  gflags::SetUsageMessage("Bridge to an Ableton Link session. Sample usage:\n" + std::string(argv[0]) +
                          " --port 1234 --poll 10");
  gflags::SetVersionString(version);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (argc > 1) {
    std::cerr << "Unrecognized argument, " << argv[1] << std::endl;
    gflags::ShowUsageWithFlagsRestrict(argv[0], "carabiner.cpp");
    return 1;
  }

  linkInstance.setTempoCallback(tempoCallback);
  linkInstance.setNumPeersCallback(peersCallback);
  linkInstance.setStartStopCallback(startStopCallback);
  linkInstance.enableStartStopSync(syncStartStop);
  linkInstance.enable(true);

  struct mg_mgr mgr;
  std::string port = "tcp://127.0.0.1:" + std::to_string(FLAGS_port);

  mg_mgr_init(&mgr, NULL);
  mg_bind(&mgr, port.c_str(), eventHandler);

  std::cout << "Starting Carabiner on port " << port << std::endl;

  for (;;) {
    std::cout << "Link bpm: " << linkInstance.captureAppSessionState().tempo() <<
      " Peers: " << linkInstance.numPeers() <<
      " Connections: " << activeConnections.size() << "     \r" << std::flush;

    mg_mgr_poll(&mgr, FLAGS_poll);
  }
  mg_mgr_free(&mgr);

  return 0;
}
