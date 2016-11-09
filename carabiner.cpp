#include <string>
#include <set>
#include <mutex>

#include <gflags/gflags.h>

#include <ableton/Link.hpp>

extern "C" {
#include "mongoose.h"
}

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

// Send a message describing the current state of the Link session.
static void reportStatus(struct mg_connection *nc) {
  ableton::Link::Timeline timeline = linkInstance.captureAppTimeline();
  std::string response = "status { :peers " + std::to_string(linkInstance.numPeers()) +
    " :bpm " + std::to_string(timeline.tempo()) +
    " :start " + std::to_string(timeline.timeAtBeat(0.0, 4.0).count()) + " }";
  mg_send(nc, response.data(), response.length());
}

// Process a request for the current link session status. Can simply remove the connection
// from the set of those which have current status updates, and one will be sent on the next
// poll.
static void handleStatus(std::string args, struct mg_connection *nc) {
  updatedMutex.lock();
  updatedConnections.erase(nc);
  updatedMutex.unlock();
}

// Process a request to establish a specific tempo
static void handleBpm(std::string args, struct mg_connection *nc) {
  std::stringstream ss(args);
  double bpm;

  ss >> bpm;
  if (ss.fail() || (bpm < 0.9) || (bpm > 400.0)) {
    // Unparsed bpm, report error
    std::string response = "bad-bpm " + args;
    mg_send(nc, response.data(), response.length());
    std::cerr << "Failed to parse bpm: " << args << std::endl;
  }  else {
    ableton::Link::Timeline timeline = linkInstance.captureAppTimeline();
    timeline.setTempo(bpm, linkInstance.clock().micros());
    linkInstance.commitAppTimeline(timeline);
    // No neeed to call reportStatus(nc) because if the BPM changed that will happen on its own.
  }
}

// Process a request to query the timeline
static void handleBeatAtTime(std::string args, struct mg_connection *nc) {
  std::stringstream ss(args);
  long when;
  double quantum;

  ss >> when;
  if (ss.fail()) {
    // Unparsed microsecond value, report error
    std::string response = "bad-time " + args;
    mg_send(nc, response.data(), response.length());
  } else {
    ss >> quantum;
    if (ss.fail() || (quantum < 2.0) || (quantum > 16.0)) {
      // Unparsed quantum value, report error
      std::string response = "bad-quantum " + args;
      mg_send(nc, response.data(), response.length());
    } else {
      ableton::Link::Timeline timeline = linkInstance.captureAppTimeline();
      double beat = timeline.beatAtTime(std::chrono::microseconds(when), quantum);
      std::string response = "beat-at-time { :when " + std::to_string(when) +
        " :quantum " + std::to_string(quantum) +
        " :beat " + std::to_string(beat) + " }";
      mg_send(nc, response.data(), response.length());
    }
  }
}

// Process a request to realign the timeline
static void handleForceBeatAtTime(std::string args, struct mg_connection *nc) {
  std::stringstream ss(args);
  double beat;
  long when;
  double quantum;

  ss >> beat;
  if (ss.fail()) {
    // Unparsed beat value, report error
    std::string response = "bad-beat " + args;
    mg_send(nc, response.data(), response.length());
  } else {
    ss >> when;
    if (ss.fail()) {
      // Unparsed microsecond value, report error
      std::string response = "bad-time " + args;
      mg_send(nc, response.data(), response.length());
    } else {
      ss >> quantum;
      if (ss.fail() || (quantum < 2.0) || (quantum > 16.0)) {
        // Unparsed quantum value, report error
        std::string response = "bad-quantum " + args;
        mg_send(nc, response.data(), response.length());
      } else {
        ableton::Link::Timeline timeline = linkInstance.captureAppTimeline();
        timeline.forceBeatAtTime(beat, std::chrono::microseconds(when), quantum);
        linkInstance.commitAppTimeline(timeline);
        reportStatus(nc);
      }
    }
  }
}

// Check whether the packet contains the specified command; if so, remove the
// command prefix from the arguments.
static bool matchesCommand(std::string msg, std::string cmd, std::string& args) {
  if (msg.substr(0, cmd.length()) == cmd) {
    //std::cout << "  is " << cmd << "command!" << std::endl;
    args = msg.substr(cmd.length(), std::string::npos);
    return true;
  }
  return false;
}

// When a packet has been received, identify the command it contains, and handle it appropriately.
static void processMessage(std::string msg, struct mg_connection *nc) {
  //std::cout << std::endl << "received: " << msg << std::endl;

  std::string args;
  if (matchesCommand(msg, "bpm ", args)) {
    handleBpm(args, nc);
  } else if (matchesCommand(msg, "beat-at-time ", args)) {
    handleBeatAtTime(args, nc);
  } else if (matchesCommand(msg, "force-beat-at-time ", args)) {
    handleForceBeatAtTime(args, nc);
  } else if (matchesCommand(msg, "status", args)) {
    handleStatus(args, nc);
  } else {
    // Unrecognized input, report error
    std::string response = "unsupported " + msg;
    mg_send(nc, response.data(), response.length());
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

int main(int argc, char* argv[]) {
  gflags::SetUsageMessage("Bridge to an Ableton Link session. Sample usage:\n" + std::string(argv[0]) +
                          " --port 1234 --poll 10");
  gflags::SetVersionString("0.1.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  if (argc > 1) {
    std::cerr << "Unrecognized argument, " << argv[1] << std::endl;
    gflags::ShowUsageWithFlagsRestrict(argv[0], "carabiner.cpp");
    return 1;
  }

  linkInstance.setTempoCallback(tempoCallback);
  linkInstance.setNumPeersCallback(peersCallback);
  linkInstance.enable(true);

  struct mg_mgr mgr;
  const char *port = ("tcp://127.0.0.1:" + std::to_string(FLAGS_port)).c_str();

  mg_mgr_init(&mgr, NULL);
  mg_bind(&mgr, port, eventHandler);

  std::cout << "Starting Carabiner on port " << port << std::endl;

  for (;;) {
    std::cout << "Link bpm: " << linkInstance.captureAppTimeline().tempo() <<
      " Peers: " << linkInstance.numPeers() <<
      " Connections: " << activeConnections.size() << "     \r" << std::flush;

    mg_mgr_poll(&mgr, FLAGS_poll);
  }
  mg_mgr_free(&mgr);

  return 0;
}
