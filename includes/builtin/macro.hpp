#ifndef BUILTIN_MACRO_H
#define BUILTIN_MACRO_H

#include <jsapi.h>

#define JSNATIVE(name) bool name(JSContext *ctx, unsigned argc, JS::Value *vp)

static JSNATIVE(noop) {
  return true;
}
#endif

