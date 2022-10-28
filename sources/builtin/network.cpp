#include <mutex>
#include <thread>
#include <enet/enet.h>

#include "binding/enet.hpp"
#include "builtin/macro.hpp"
#include "builtin/network.hpp"
#include "event_loop/loop.hpp"

namespace builtin {
  namespace network {
    static std::mutex mx;
    static bool       shutdown = false;

    JS::Heap<JSObject*> handler;

    void traceHandler(JSTracer* tracer, void* data) {
      JS::TraceEdge(tracer, &handler, "net::handler");
    };

    JSNATIVE(getHandler) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      args.rval().setObjectOrNull(*handler.address());
      return true;
    }

    JSNATIVE(setHandler) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      handler.set(&args.get(0).toObject());
      args.rval().setUndefined();
      return true;
    }
    
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
    JSNATIVE(postMessage) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      args.rval().setUndefined();
      return true;
    }

    class ENetTask : public event_loop::Task {
            ENetHost            *host;
            ENetEvent           *event;
            JSContext           *ctx;
            JS::Heap<JSObject*> functionRef;
            public:
                    ENetTask(ENetHost *host, ENetEvent *event, JSContext *ctx, JS::Heap<JSObject*> &functionRef) :
                            host(host),
                            event(event),
                            ctx(ctx),
                            functionRef(functionRef)
                    {}
                    bool ready() override {
                            return true;
                    }

                    void run() override {
                            JS::RootedObject global(ctx, JS::CurrentGlobalOrNull(ctx));
                            JS::RootedValue handler(ctx, JS::ObjectOrNullValue(functionRef.get()));

                            JS::RootedObject clientHost(ctx, JS_NewObject(ctx, &enet::hostClass)),
                                             eventObject(ctx, JS_NewObject(ctx, &enet::eventClass));

                            JS_DefineProperties(ctx, clientHost,  enet::hostProperties);
                            JS_DefineProperties(ctx, eventObject, enet::eventProperties);
                            JS_DefineFunctions(ctx,  clientHost,  enet::hostMethods);
                            JS_DefineFunctions(ctx,  eventObject, enet::eventMethods);

                            JS_SetPrivate(clientHost, host);
                            JS_SetPrivate(eventObject, &event);

                            // finally call the function associated
                            JS::RootedValueArray<2> args(ctx);
                            args[0].setObject(*clientHost);
                            args[1].setObject(*eventObject);
                            JS::RootedValue rval(ctx);
                            JS_CallFunctionValue(ctx, global, handler, args, &rval);

                            if (JS_IsExceptionPending(ctx)) {
                                    JS::ExceptionStack exnStack(ctx);
                                    JS::StealPendingExceptionStack(ctx, &exnStack);
                                    JS::ErrorReportBuilder builder(ctx);
                                    builder.init(ctx, exnStack, JS::ErrorReportBuilder::NoSideEffects);
                                    JS::PrintError(ctx, stderr, builder, false);
                            }
                    }
    };

    void startEnet(JSContext *ctx, ENetHost *host) {
            shutdown = false;
            auto loop = static_cast<event_loop::Loop*>(JS_GetContextPrivate(ctx));

            while (!shutdown) {
                    //TODO(edoput) configurable timeout
                    int timeout = 1000;
                    auto event = static_cast<ENetEvent*>(malloc(sizeof(ENetEvent)));
                    while (enet_host_service(host, event, timeout) > 0) {
                            auto t = new ENetTask(host, event, ctx, handler);
                            loop->queue(t);
                    }
            }
    }

    JSNATIVE(start) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

      ENetAddress address;
      enet_address_set_host(&address, "127.0.0.1");
      address.port = 20595;
      auto host = enet_host_create(&address, 32, 2, 0, 0);

      std::thread m(startEnet, ctx, host);
      m.detach();
      args.rval().setUndefined();
      return true;
    }

    JSNATIVE(stop) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      shutdown = true;
      args.rval().setUndefined();
      return true;
    }

    JSNATIVE(connect) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      ENetAddress address;
      enet_address_set_host(&address, "127.0.0.1");
      address.port = 20595;
      auto host = enet_host_create(NULL, 1, 2, 0, 0);
      enet_host_connect(host, &address, 2, 0);
      args.rval().setUndefined();
      return true;
    }

    // exposes globally the name _netserver_ for an object of class netserver
    void withNetwork(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
      JS::RootedObject result(ctx, JS_DefineObject(ctx, global, "network", &serverClass, 0));
      JS_DefineProperties(ctx, result, serverProperties);
      JS_DefineFunctions(ctx, result, serverMethods);
      out.set(result);
      JS_AddExtraGCRootsTracer(ctx, traceHandler, nullptr);
    }
  };
}
