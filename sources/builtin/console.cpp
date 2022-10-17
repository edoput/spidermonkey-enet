#include<iostream>
#include <mutex>

#include <jsapi.h>
#include <js/Conversions.h>

#include "builtin/macro.hpp"
#include "builtin/console.hpp"

namespace builtin {
  namespace console {
    static std::mutex m;

    JSNATIVE(consoleLog) {
      JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
      std::scoped_lock<std::mutex> lk{m};
      //TODO(edoput) argc == args.length() ???
      for (unsigned i = 0; i < argc; i++) {
          std::cout << JS_EncodeStringToASCII(ctx, JS::ToString(ctx, args.get(i))).get();
      }
      std::cout << '\n';
      return true;
    }
    
    void withConsole(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
      JS::RootedObject result(ctx, JS_DefineObject(ctx, global, "console", &consoleClass, 0));
      JS_DefineFunctions(ctx, result, consoleMethods);
      out.set(result);
    }
  };
};
