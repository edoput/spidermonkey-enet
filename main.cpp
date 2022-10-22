#include <cassert>
#include <string>
//#include <fstream>
//#include <iostream>
//
#include <jsapi.h>
//#include <enet/enet.h>
//
#include <js/CompilationAndEvaluation.h>
#include <js/CompileOptions.h>
//#include <js/Conversions.h>
#include <js/Initialization.h>
//#include <js/SourceText.h>
#include <js/Warnings.h>

#include "base.cpp"
#include "builtin/console.hpp"
#include "builtin/network.hpp"
#include "builtin/timeout.hpp"

using string = std::string;

// now you can use JS::WarnUTF8 and friends from C++ and it will
// be logged to this file.
FILE *log_destination;

void reporter(JSContext* ctx, JSErrorReport *report) {
  JS::PrintError(ctx, log_destination, report, true);
  fflush(log_destination);
}

/** The JS interpreter needs a global object. You can attach
 other names to the global objects like the browser does with
 _window_, _document_ and so on.
*/
namespace global {
  static JSClass globalClass = {
    "global",
    JSCLASS_GLOBAL_FLAGS,
    &JS::DefaultGlobalClassOps,
  };
};

int main(int argc, const char* argv[]) {
  
  string role{argv[0]};
  
  Setup();

  JSContext *rootCtx = JS_NewContext(JS::DefaultHeapMaxBytes);
  //JSContext *enetCtx = JS_NewContext(JS::DefaultHeapMaxBytes);


  //TODO(edoput) enable promises
  //js::UseInternalJobQueue(rootCtx); // Promises
  JS::InitSelfHostedCode(rootCtx);
  //JS::InitSelfHostedCode(enetCtx);

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
  JS::InitRealmStandardClasses(rootCtx);
  JS::RootedObject network(rootCtx);
  JS::RootedObject console(rootCtx);
  JS::RootedObject timeout(rootCtx);

  builtin::network::withNetwork(rootCtx, global, &network);
  builtin::console::withConsole(rootCtx, global, &console);
  builtin::timeout::withTimeout(rootCtx, global, &timeout);

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
