#include "event_loop/loop.hpp"

namespace event_loop {
        void Loop::queue(Task* t) {
                m_queue.push(t);
        }

        std::optional<Task*> Loop::next() {
                std::optional<Task*> readyTask = {};
                return readyTask;
        }
};
