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

  JSContext *rootCtx = JS_NewContext(JS::DefaultHeapMaxBytes);
  JSContext *enetCtx = JS_NewContext(JS::DefaultHeapMaxBytes, JS_GetRuntime(rootCtx));


  //TODO(edoput) enable promises
  //js::UseInternalJobQueue(rootCtx); // Promises
  JS::InitSelfHostedCode(rootCtx);
  JS::InitSelfHostedCode(enetCtx);

  if (role == "server") {
    log_destination = fopen("spidermonkey-client.log", "w+");
  } else {
    log_destination = fopen("spidermonkey-server.log", "w+");
  }
  assert(!JS::SetWarningReporter(rootCtx, reporter));

  // https://groups.google.com/g/mozilla.dev.tech.js-engine/c/wr55L3lCMZg/m/wExXgUqvAgAJ
  // Yes you can create multiple global objects with JS_NewGlobalObject, switch
  // between them with JSAutoRealm, and compile/execute code in each
  // realm/global separately.  
  JS::RealmOptions options;
  JS::RootedObject global(rootCtx, JS_NewGlobalObject(rootCtx, &global::globalClass, nullptr, JS::FireOnNewGlobalHook, options));

  JSAutoRealm ar(rootCtx, global);
  //TODO(edoput) built-in JS global object properties are already initialized
  //JS::InitRealmStandardClasses(rootCtx);
  JS::RootedObject network(rootCtx);
  JS::RootedObject console(rootCtx);

  builtin::network::withNetwork(rootCtx, global, &network);
  builtin::console::withConsole(rootCtx, global, &console);

  /* evaluate the script */  
  JS::RootedValue    rval(rootCtx);
  // the script must set the netserver.handler to a function
  if (!LoadScript(rootCtx, "network.js", &rval)) {
    if (JS_IsExceptionPending(rootCtx)) {
      JS::ExceptionStack exnStack(rootCtx);
      JS::StealPendingExceptionStack(rootCtx, &exnStack);
      JS::ErrorReportBuilder builder(rootCtx);
      builder.init(rootCtx, exnStack, JS::ErrorReportBuilder::NoSideEffects);
      JS::PrintError(rootCtx, stderr, builder, false);
    }
    return -1;
  }

  if (role == "./client") {
    ENetHost *client = MakeClient();
    Client(client, rootCtx, global, network, console);
  } else {
    ENetHost  *server = MakeHost();
    // set up an internal job queue for Promises
    // before the self-hosting
    Server(server, rootCtx, global, network, console);
  }

  JS_DestroyContext(rootCtx);

  Teardown();
}
