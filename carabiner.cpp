#include <ableton/Link.hpp>

extern "C" {
  #include "mongoose.h"
}

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
  struct mbuf *io = &nc->recv_mbuf;
  (void) p;
  switch (ev) {
    case MG_EV_RECV:
      mg_send(nc, io->buf, io->len);  // Echo message back
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
  struct mg_mgr mgr;
  const char *port1 = "udp://1234", *port2 = "udp://127.0.0.1:17000";
  ableton::Link link(120.);

  mg_mgr_init(&mgr, NULL);
  mg_bind(&mgr, port1, ev_handler);
  mg_bind(&mgr, port2, ev_handler);

  link.enable(true);

  std::cout << "Starting echo mgr on ports " << port1 << ", " << port2 << std::endl;

  for (;;) {
    std::cout << "Link bpm: " << link.captureAppTimeline().tempo() << "     \r" << std::flush;

    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}
