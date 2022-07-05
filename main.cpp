#include <string>
#include <fstream>
#include <iostream>

#include <jsapi.h>
#include <enet/enet.h>

#include <js/CompilationAndEvaluation.h>
#include <js/CompileOptions.h>
#include <js/Conversions.h>
#include <js/Initialization.h>
#include <js/SourceText.h>


#define JSNATIVE(name) bool name(JSContext *ctx, unsigned argc, JS::Value *vp)
#include "base.cpp"

JSNATIVE(noop) {
  return true;
}

using string = std::string;

/* you cannot store just JSFunction because that;s just the code! What you
 * want instead is the function value and the closure which means keeping around
 * the full JSObject*.
 * Moreover when we call netserver.addEventListener(eventName, listener);
 * we have to take ownership of listener as a JSObject. This means adding a
 * callback to the Spidermonkey GC to trace the JSObject* we have and mark
 * them as active.
 */
#if 0
static std::vector<std::pair<JS::Heap<JSString*>, JS::Heap<JSFunction*>>> listeners = {};

void trace_listeners(JSTracer* tracer, void *data) {
  for (auto &element : listeners) {
    JS::TraceEdge(tracer, &(element.first), "eventName");
    JS::TraceEdge(tracer, &(element.second), "eventHandler");
  }
}


JSNATIVE(addEventListener) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  auto eventName = JS::ToString(ctx, args.get(0));
  auto listenerFunc = JS_ValueToFunction(ctx, args.get(1));
  listeners.push_back(std::make_pair(JS::Heap(eventName), JS::Heap(listenerFunc)));
  return true;
}

JSNATIVE(removeEventListener) {
  return true;  
}
#endif

JSNATIVE(consoleLog) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  JS::RootedString rval_str(ctx, JS::ToString(ctx, args.get(0)));
  std::cout << JS_EncodeStringToASCII(ctx, rval_str).get() << '\n';
  return true;
}

namespace global {
static JSClass globalClass = {
  "global",
  JSCLASS_GLOBAL_FLAGS,
  &JS::DefaultGlobalClassOps,
};

};

namespace net {

class NetServer {
  ENetHost* _host;
  JS::Heap<JSFunction*> onConnect;
  JS::Heap<JSFunction*> onDisconnect;
  JS::Heap<JSFunction*> onMessage;
  
  void trace(JSTracer* tracer, void *data) {
    JS::TraceEdge(tracer, &onConnect, "onConnect");
    JS::TraceEdge(tracer, &onDisconnect, "onDisconnect");
    JS::TraceEdge(tracer, &onMessage, "onMessage");
  } 
};

#if 0
JSNATIVE(dispatch) {
  JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
  auto dispatchEventName = args.get(0);
  JS::RootedValue rval(ctx);
  JS::RootedValueArray<2> handlerArgs(ctx);
  handlerArgs[0].set(args.get(1));
  handlerArgs[1].set(args.get(2));
  for (auto p : listeners) {
    //TODO(edoput) compare eventName with dispatchEventName
    //TODO(edoput) Heap<T>.address() gives back a pointer, we need to convert to handle
    JS::RootedFunction fval(ctx, p.second);
    JS_CallFunction(ctx, nullptr, fval, handlerArgs, &rval);
  }
  return true;
}
#endif

static JSClass serverClass = {
  "netserver",
  0
};
static JSPropertySpec serverProperties[] = {};
static JSFunctionSpec serverMethods[] = {
    JS_FN("dispatch", noop, 2, 0),
    JS_FS_END,
};
  void withNetServer(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
    JS::RootedObject result(ctx, JS_DefineObject(ctx, global, "netserver", &serverClass, 0));
    //TODO(edoput) get netserver object
    //TODO(edoput) define netserver properties
    JS_DefineFunctions(ctx, result, serverMethods);
    //TODO(edoput) define private data
    //TODO(edoput) allocate NetServer class
    //TODO(edoput) set instance as private data
    //TODO(edoput) hook up class to spidermonkey tracing GC
    //TODO(edoput) 
    // trace also the stuff we add to _listeners_
    // JS_AddExtraGCRootsTracer(ctx, &trace_listeners, nullptr);
    out.set(result);
  }
};


namespace console {
static JSClass consoleClass = {
  "console",
  0
};

static JSFunctionSpec consoleMethods[] = {
  JS_FN("log", consoleLog, 1, 0),
  JS_FS_END,
};
  void withConsole(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
    JS::RootedObject result(ctx, JS_DefineObject(ctx, global, "console", &consoleClass, 0));
    JS_DefineFunctions(ctx, result, consoleMethods);
    out.set(result);
  }
};

int main(int argc, const char* argv[]) {
  
  string part = string(argv[0]);
  
  Setup();

  JSContext *ctx = JS_NewContext(JS::DefaultHeapMaxBytes);

  JS::InitSelfHostedCode(ctx);
  JS::RealmOptions options;
  JS::RootedObject global(ctx, JS_NewGlobalObject(ctx, &global::globalClass, nullptr, JS::FireOnNewGlobalHook, options));

  JSAutoRealm ar(ctx, global);
  //TODO(edoput) built-in JS global object properties are already initialized
  //JS::InitRealmStandardClasses(ctx);
  JS::RootedObject netserver(ctx);
  JS::RootedObject console(ctx);
  
  net::withNetServer(ctx, global, &netserver);
  console::withConsole(ctx, global, &console);


  /* evaluate the script */  
  JS::RootedValue    rval(ctx);
  if (!LoadScript(ctx, "network.js", &rval)) {

    if (JS_IsExceptionPending(ctx)) {
      JS::RootedValue exc(ctx);
      if (!JS_GetPendingException(ctx, &exc)) {
	std::cout << "could not get exception\n";
	return -1;
      }
      PrintResult(ctx, rval, "(error:networks.js): ");
    }
    std::cout << "no exception thrown by network.js\n";
    return -1;
  }
  PrintResult(ctx, rval, "(network.js): ");
  //TODO(edoput)
  //JS::RootedFunction dispatchEvent(ctx, JS_NewFunction(ctx, dispatch, 2, 0, "dispatchEvent"));
  if (part == "./client") {
    ENetHost *client = MakeClient();
    Client(client, ctx, global, dispatchEvent);
  } else {
    ENetHost  *server = MakeHost();
    // set up an internal job queue for Promises
    // before the self-hosting
    Server(server, ctx, global, dispatchEvent);
  }

  JS_DestroyContext(ctx);

  Teardown();
}
