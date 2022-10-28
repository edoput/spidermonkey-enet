#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <condition_variable>
#include <functional>
#include <optional>
#include <list>

#include <jsapi.h>

namespace event_loop {
        //class Event {};
        //class EventTarget {};

        /** Formally a task is a struct which has:
         * - Steps: a series of steps specifying the work to be done by the task.
         * - A source: one of the task sources, used to group and serialize related tasks.
         * - A document: a document associated with the task, or null for tasks that are
         *   not in a window event loop.
         * - A script evaluation environment settings object set: A set of environment
         *   settings object used for tracking script evaluation during the task.
         */

        //using Steps = std::function<void>(void);
        //using Source = class {};
        //using Document = class {};

        //using Realm = class {};
        //using ModuleMap = class {};

        //class Environment {
        //        Realm     realm;
        //        ModuleMap modulemap;
        //};

        class Task {
                //Steps       steps;
                //Source      source;
                //Document    document;
                //Environment environemnt;

                public:
                        // a task is runnable if its document is either null or fully active
                        virtual bool ready() = 0;
                        virtual void run() = 0;
        };

        /** Each event loop has a currently running task, which is either a task or null.
         * Initially this is null. It is used to handle reentrancy.
         */

        /* 8.1.7.2 Queuing tasks algorithm */

        using TaskQueue = std::list<Task*>;

        class Loop {
                public:
                        Loop();
                        ~Loop();
                        void queue(Task* t);
                        std::optional<Task*> next();
                private:
                        TaskQueue m_queue;
        };
};
#endif
