#ifndef BUILTIN_TIMEOUT_H
#define BUILTIN_TIMEOUT_H

#include "builtin/macro.hpp"
#include <jsapi.h>

namespace builtin {
  namespace timeout {
    void withTimeout(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out);
  };
};
#endif
