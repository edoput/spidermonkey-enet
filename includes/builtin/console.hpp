#ifndef BUILTIN_CONSOLE_H
#define BUILTIN_CONSOLE_H

#include "builtin/macro.hpp"
#include <jsapi.h>

namespace builtin {
  namespace console {
    static JSClass consoleClass = {
      "console",
      0
    };

    JSNATIVE(consoleLog);

    static JSFunctionSpec consoleMethods[] = {
      JS_FN("log", consoleLog, 1, 0),
      JS_FS_END,
    };

    void withConsole(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out);
  };
};
#endif
