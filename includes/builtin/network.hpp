#ifndef BUILTIN_NETWORK_H
#define BUILTIN_NETWORK_H
#include "builtin/macro.hpp"
#include <jsapi.h>
namespace builtin {
  namespace network {

    static JSClass serverClass = {
      "Network",
      0
    };

    JSNATIVE(getHandler);

    JSNATIVE(setHandler);
    
    static JSPropertySpec serverProperties[] = {
      JS_PSGS("handler", getHandler, setHandler, 0),
      JS_PS_END,
    };

    /** postMessage is exposed on network.
     * I need to make sure always creating a ENetHost is intended
     * and I have a pointer for it.
     *
     * The server is intended to receive packets from the clients
     * and forward them. This is accomplished using enet_host_broadcast.
     *
     * A client is intended to send and receive packets from the server.
     * This is accomplished using enet_peer_send or enet_host_broadcast?
     * As long as a client only holds one peer in its connections would
     * they work the same?
     */
    JSNATIVE(postMessage);
    JSNATIVE(start);
    JSNATIVE(stop);
    JSNATIVE(connect);

    static JSFunctionSpec serverMethods[] = {
      JS_FN("postMessage", postMessage, 1, 0),
      JS_FN("start", start, 1, 0),
      JS_FN("stop", stop, 1, 0),
      JS_FN("connect", connect, 1, 0),
      JS_FS_END,
    };

    // exposes globally the name _netserver_ for an object of class netserver
    void withNetwork(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out);
  };
};
#endif
