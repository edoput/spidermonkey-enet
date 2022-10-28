#include <cstdio>
#include <cassert>
#include <string>
#include <filesystem>

#include <jsapi.h>
#include <js/CompilationAndEvaluation.h>
#include <js/CompileOptions.h>
#include <js/Initialization.h>
#include <js/Warnings.h>
#include <enet/enet.h>

#include "builtin/console.hpp"
#include "builtin/network.hpp"
#include "builtin/timeout.hpp"

#include "host/agent.hpp"
#include "event_loop/loop.hpp"

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

event_loop::Loop loop;

class ScriptTask : public event_loop::Task {

        using Path = std::filesystem::path;

        JSContext* ctx;
        Path       scriptPath;

        public:
                ScriptTask(JSContext* ctx, Path scriptPath) :
                        ctx(ctx),
                        scriptPath(scriptPath)
                {}

                bool ready() override {
                        return true;
                };

                void run() override {
                        JS::OwningCompileOptions options(ctx);
                        JS::RootedValue rval(ctx);;
                        JS::EvaluateUtf8Path(ctx, options, scriptPath.c_str(), &rval);
                        if (JS_IsExceptionPending(ctx)) {
                          JS::ExceptionStack exnStack(ctx);
                          JS::StealPendingExceptionStack(ctx, &exnStack);
                          JS::ErrorReportBuilder builder(ctx);
                          builder.init(ctx, exnStack, JS::ErrorReportBuilder::NoSideEffects);
                          JS::PrintError(ctx, stderr, builder, false);
                        }
                };
                // conversion to task
                //operator event_loop::Task*() {
                //        return dynamic_cast<event_loop::Task*>(this);
                //}
};

int main(int argc, const char* argv[]) {
  
  //try {
  //      host::agent::init();
  //} catch (std::exception e) {
  //      printf(e.what());
  //}
  JS_Init();
  enet_initialize();
  atexit(enet_deinitialize);

  JSContext *rootCtx = JS_NewContext(JS::DefaultHeapMaxBytes);
  //JSContext *enetCtx = JS_NewContext(JS::DefaultHeapMaxBytes);


  //TODO(edoput) enable promises
  //js::UseInternalJobQueue(rootCtx); // Promises
  JS::InitSelfHostedCode(rootCtx);
  //JS::InitSelfHostedCode(enetCtx);

  //if (role == "server") {
  //  log_destination = fopen("spidermonkey-client.log", "w+");
  //} else {
  //  log_destination = fopen("spidermonkey-server.log", "w+");
  //}
  //assert(!JS::SetWarningReporter(rootCtx, reporter));

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

  // attach to this context the event loop
  JS_SetContextPrivate(rootCtx, static_cast<void*>(&loop));

  string scriptName{argv[1]};
  auto networkScript = new ScriptTask(rootCtx, scriptName);

  loop.queue(networkScript);
  while (true) {
          // NOTE(edoput) I don't like this implementation right now
          // the loop just spin always and there is no indication of sending
          // the thread to sleep at least for a little

          // MACRO task
          if (auto task = loop.next()) {
                  task.value()->run();
          }
          // TODO(edoput) MICRO task
  }

  JS_DestroyContext(rootCtx);

}
