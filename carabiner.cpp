#include <ableton/Link.hpp>
#include <string>
#include <set>
#include <mutex>

extern "C" {
#include "mongoose.h"
}

ableton::Link linkInstance(120.);

std::set<struct mg_connection *> activeConnections, updatedConnections;
std::mutex updatedMutex;

// Send a response describing the current state of the Link timeline.
static void report_status(struct mg_connection *nc) {
  ableton::Link::Timeline timeline = linkInstance.captureAppTimeline();
  std::string response = "status { :peers " + std::to_string(linkInstance.numPeers()) +
    " :bpm " + std::to_string(timeline.tempo()) +
    " :start " + std::to_string(timeline.timeAtBeat(0.0, 4.0).count()) + " }";
  mg_send(nc, response.data(), response.length());
}

static void handle_status(std::string args, struct mg_connection *nc) {
  report_status(nc);
}

static void handle_bpm(std::string args, struct mg_connection *nc) {
  std::stringstream ss(args);
  double bpm;

  ss >> bpm;
  if (ss.fail()) {
    // Unparsed bpm, report error
    std::string response = "bad_bpm " + args;
    mg_send(nc, response.data(), response.length());
    std::cout << "Failed to parse bpm: " << args << std::endl;
  }  else {
    ableton::Link::Timeline timeline = linkInstance.captureAppTimeline();
    timeline.setTempo(bpm, linkInstance.clock().micros());
    linkInstance.commitAppTimeline(timeline);
    report_status(nc);
  }
}

static void handle_beat(std::string args, struct mg_connection *nc) {
  std::stringstream ss(args);
  // TODO: read beat number, bar size, latency values
  ableton::Link::Timeline timeline = linkInstance.captureAppTimeline();
  timeline.forceBeatAtTime(0.0, linkInstance.clock().micros(), 4.0);
  linkInstance.commitAppTimeline(timeline);
  report_status(nc);
}

static bool matches_command(std::string msg, std::string cmd, std::string& args) {
  if (msg.substr(0, cmd.length()) == cmd) {
    std::cout << "  is " << cmd << "command!" << std::endl;
    args = msg.substr(cmd.length(), std::string::npos);
    return true;
  }
  return false;
}

static void process_message(std::string msg, struct mg_connection *nc) {
  std::cout << std::endl << "received: " << msg << std::endl;

  std::string args;
  if (matches_command(msg, "bpm ", args)) {
    handle_bpm(args, nc);
  } else if (matches_command(msg, "beat ", args)) {
    handle_beat(args, nc);
  } else if (matches_command(msg, "status", args)) {
    handle_status(args, nc);
  } else {
    // Unrecognized input, report error
    std::string response = "unsupported " + msg;
    mg_send(nc, response.data(), response.length());
  }
}

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
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
      report_status(nc);
    }
    break;

  case MG_EV_RECV:
    process_message(std::string(io->buf, io->len), nc);
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

int main(void) {
  linkInstance.setTempoCallback(tempoCallback);
  linkInstance.setNumPeersCallback(peersCallback);
  linkInstance.enable(true);

  struct mg_mgr mgr;
  const char *port = "tcp://127.0.0.1:17000";

  mg_mgr_init(&mgr, NULL);
  mg_bind(&mgr, port, ev_handler);

  std::cout << "Starting Carabiner on port " << port << std::endl;

  for (;;) {
    std::cout << "Link bpm: " << linkInstance.captureAppTimeline().tempo() <<
      " Peers: " << linkInstance.numPeers() <<
      " Connections: " << activeConnections.size() << "     \r" << std::flush;

    mg_mgr_poll(&mgr, 20);
  }
  mg_mgr_free(&mgr);

  return 0;
}
