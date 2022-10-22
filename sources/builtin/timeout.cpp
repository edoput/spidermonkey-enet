#include <jsapi.h>

#include "builtin/macro.hpp"
#include "builtin/timeout.hpp"

#include "event_loop/loop.hpp"

namespace builtin {
        namespace timeout {
                //TODO(edoput) tracing functionRef
                class TimeoutTask : public event_loop::Task {
                        JSContext            *ctx;
                        JS::Heap<JSObject*>  functionRef;
                        //TODO(edoput) args value array in Heap pointer
                        std::chrono::time_point<std::chrono::steady_clock> target;

                public:
                        TimeoutTask(JSContext *ctx, JSObject *functionRef) :
                                ctx(ctx),
                                functionRef(functionRef),
                                target(std::chrono::steady_clock::now())
                        {}

                        TimeoutTask(JSContext *ctx, JSObject *functionRef, int delay) :
                                ctx(ctx),
                                functionRef(functionRef),
                                target(std::chrono::steady_clock::now()
                                       + std::chrono::milliseconds(delay))
                        {}

                        bool ready() override {
                                return std::chrono::steady_clock::now() >= target;
                        }

                        void run() override {
                                auto args = JS::HandleValueArray::empty();
                                JS::RootedObject global(ctx, JS::CurrentGlobalOrNull(ctx));
                                JS::RootedValue callback(ctx, JS::ObjectOrNullValue(functionRef.get()));
                                JS::RootedValue rval(ctx);

                                JS_CallFunctionValue(ctx, global, callback, args, &rval);

                                if (JS_IsExceptionPending(ctx)) {
                                        JS::ExceptionStack exnStack(ctx);
                                        JS::StealPendingExceptionStack(ctx, &exnStack);
                                        JS::ErrorReportBuilder builder(ctx);
                                        builder.init(ctx, exnStack, JS::ErrorReportBuilder::NoSideEffects);
                                        JS::PrintError(ctx, stderr, builder, false);
                                }
                        }
                };

                JSNATIVE(setTimeout) {
                        // setTimeout(functionRef, delay)
                        // setTimeout(functionRef, delay, param1);
                        // setTimeout(functionRef, delay, param1, param2);
                        // setTimeout(functionRef, delay, param1, param2, ...);
                        
                        JS::CallArgs args = JS::CallArgsFromVp(argc, vp);

                        // TODO(edoput) get timeout duration
                        auto timeout = new TimeoutTask(ctx, &args.get(0).toObject(), 0);
                        auto loop = static_cast<event_loop::Loop*>(JS_GetContextPrivate(ctx));
                        loop->queue(timeout);
                        // TODO(edoput) return id of timeout
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
