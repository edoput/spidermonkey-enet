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
#include <js/Warnings.h>


#define JSNATIVE(name) bool name(JSContext *ctx, unsigned argc, JS::Value *vp)
#include "builtin.cpp"
#include "base.cpp"

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

// now you can use JS::WarnUTF8 and friends from C++ and it will
// be logged to this file.
FILE *log_destination;

void reporter(JSContext* ctx, JSErrorReport *report) {
  JS::PrintError(ctx, log_destination, report, true);
  fflush(log_destination);
}

int main(int argc, const char* argv[]) {
  
  string role = string(argv[0]);
  
  Setup();

  JSContext *ctx = JS_NewContext(JS::DefaultHeapMaxBytes);

  JS::InitSelfHostedCode(ctx);

  if (role == "server") {
    log_destination = fopen("spidermonkey-client.log", "w+");
  } else {
    log_destination = fopen("spidermonkey-server.log", "w+");
  }
  assert(!JS::SetWarningReporter(ctx, reporter));


  JS::RealmOptions options;
  JS::RootedObject global(ctx, JS_NewGlobalObject(ctx, &global::globalClass, nullptr, JS::FireOnNewGlobalHook, options));

  JSAutoRealm ar(ctx, global);
  //TODO(edoput) built-in JS global object properties are already initialized
  //JS::InitRealmStandardClasses(ctx);
  JS::RootedObject netserver(ctx);
  JS::RootedObject console(ctx);

  builtin::network::withNetServer(ctx, global, &netserver);
  builtin::console::withConsole(ctx, global, &console);


  /* evaluate the script */  
  JS::RootedValue    rval(ctx);
  // the script must set the netserver.handler to a function
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
  if (role == "./client") {
    ENetHost *client = MakeClient();
    Client(client, ctx, global, netserver, console);
  } else {
    ENetHost  *server = MakeHost();
    // set up an internal job queue for Promises
    // before the self-hosting
    Server(server, ctx, global, netserver, console);
  }

  JS_DestroyContext(ctx);

  Teardown();
}
