#include "signalwatcher.h"

#include <csignal>


#include "forwardlist.hpp"



namespace {
static ForwardList<SignalWatcher*> subscribers;
}

SignalWatcher::SignalWatcher() :
	flags()
{
	if (subscribers.empty()) {
		std::signal(SIGWINCH, callback);
		std::signal(SIGINT, callback);
	}

	subscribers.emplace_front(this);
}

SignalWatcher::~SignalWatcher()
{
	subscribers.erase(this);

	if (subscribers.empty()) {
		std::signal(SIGWINCH, SIG_IGN);
		std::signal(SIGINT, SIG_IGN);
	}
}

bool SignalWatcher::checkAndClearFlag(WatcherEvents index)
{
	bool result = flags[(watcherEventsType_t)index].test();
	flags[(watcherEventsType_t)index].clear();

	return result;
}

void SignalWatcher::notifyWatchers(WatcherEvents flag)
{
	for (auto& watcher : subscribers) {
		watcher->flags[(watcherEventsType_t)flag].test_and_set();
	}
}

void SignalWatcher::callback(int signal) {
	switch (signal) {
		case SIGWINCH: notifyWatchers(WatcherEvents::TERMINAL_SIZE_CHANGED); return;
		case SIGINT: notifyWatchers(WatcherEvents::INTERRUPT); return;
	}
}
