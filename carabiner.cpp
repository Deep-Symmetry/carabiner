#include <ableton/Link.hpp>
#include <string>

extern "C" {
#include "mongoose.h"
}

ableton::Link link_instance(120.);

static void handle_status(std::string args, struct mg_connection *nc) {
  ableton::Link::Timeline timeline = link_instance.captureAppTimeline();
  std::string response = "status { :peers " + std::to_string(link_instance.numPeers()) +
    " :bpm " + std::to_string(timeline.tempo()) + " }";
  mg_send(nc, response.data(), response.length());
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
    ableton::Link::Timeline timeline = link_instance.captureAppTimeline();
    timeline.setTempo(bpm, link_instance.clock().micros());
    link_instance.commitAppTimeline(timeline);
    std::string response = "bpm " + std::to_string(timeline.tempo());
    mg_send(nc, response.data(), response.length());
    std::cout << "Set bpm to " << timeline.tempo() << std::endl;
  }
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
  } else if (matches_command(msg, "status ", args)) {
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
  switch (ev) {
  case MG_EV_RECV:
    process_message(std::string(io->buf, io->len), nc);
    mbuf_remove(io, io->len);       // Discard message from recv buffer
    // In case of UDP, Mongoose creates new virtual connection for
    // incoming messages
    // We can keep it (and it will be reused for another messages from
    // the same address) or we can close it (this saves some memory, but
    // decreases perfomance, because it forces creation of connection
    // for every incoming dgram)
    nc->flags |= MG_F_SEND_AND_CLOSE;
    break;
  default:
    break;
  }
}

int main(void) {
  link_instance.enable(true);

  struct mg_mgr mgr;
  const char *port1 = "udp://1234", *port2 = "udp://127.0.0.1:17000";

  mg_mgr_init(&mgr, NULL);
  mg_bind(&mgr, port1, ev_handler);
  mg_bind(&mgr, port2, ev_handler);

  std::cout << "Starting echo mgr on ports " << port1 << ", " << port2 << std::endl;

  for (;;) {
    std::cout << "Link bpm: " << link_instance.captureAppTimeline().tempo() << "     \r" << std::flush;

    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}
