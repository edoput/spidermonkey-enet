#include "event_loop/loop.hpp"

namespace event_loop {
        Loop::Loop() {
                m_queue = {};
        }
        Loop::~Loop() {
                //TODO(edoput) LEAK
                m_queue.clear();
        }
        void Loop::queue(Task* t) {
                m_queue.push_front(t);
        }

        std::optional<Task*> Loop::next() {
                std::optional<Task*> readyTask = {};
                if (!m_queue.empty()) {
                        // scan the queue in order from most recent to least recent
                        for (auto t : m_queue) {
                                if (t->ready()) {
                                        readyTask = t;
                                        break;
                                }
                        }
                        if (readyTask.has_value()) {
                                m_queue.remove(readyTask.value());
                        }
                }
                return readyTask;
        }
};
