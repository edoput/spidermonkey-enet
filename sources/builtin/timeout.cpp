#include "builtin/macro.hpp"
#include "builtin/timeout.hpp"

#include "event_loop/event_loop.hpp"

namespace builtin {
        namespace timeout {
                // TODO(edoput) inherit from Task
                class TimeoutTask {
                        std::chrono::time_point<std::chrono::steady_clock> deadline;

                        public:
                                TimeoutTask(JSObject *functionRef, int ms) {
                                        auto now = std::chrono::steady_clock::now();
                                        deadline = now + std::chrono::milliseconds(ms);
                                }

                                bool runnable() {
                                        return std::chrono::steady_clock::now() >= deadline;
                                }

                                void run() {
                                        return;
                                }
                };

                JSNATIVE(setTimeout) {
                        // setTimeout(functionRef, delay)
                        // setTimeout(functionRef, delay, param1);
                        // setTimeout(functionRef, delay, param1, param2);
                        // setTimeout(functionRef, delay, param1, param2, ...);
                        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
                        JS::HandleValue functionRef = args.get(0);

                        // TODO(edoput) get timeout duration
                        new TimeoutTask(nullptr, 0);
                        // TODO(edoput) queue to event loop
                        // TODO(edoput) return value
                        return true;
                };

                // TODO(edoput) clearTimeout
                JSNATIVE(clearTimeout) {
                        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);
                        return true;
                };

                void withTimeout(JSContext *ctx, JS::HandleObject global, JS::MutableHandleObject out) {
                        JS_DefineFunction(ctx, global, "setTimeout", &setTimeout, 0, 0);
                        JS_DefineFunction(ctx, global, "clearTimeout", &clearTimeout, 0, 0);
                }
        };
};
