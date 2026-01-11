#pragma once

#include <array>
#include <atomic>


typedef unsigned char watcherEventsType_t;
/** @brief The list of available interupts to watch.
 *
 *  _SIZE must not be used and always be the last element.
 */
enum class WatcherEvents : watcherEventsType_t {
	TERMINAL_SIZE_CHANGED = 0,
	INTERRUPT,
	_SIZE
};

/** @brief Watches for signals and buffers flags till SignalWatcher::pullFlag
 *         call.
 *
 *  The implementation only supports UNIX signals handling. Class is fully lock
 *  free. Watching start after innitialization. Only signals after creation of
 *  variable are registered.
 */
class SignalWatcher {
public:

	SignalWatcher();
	SignalWatcher(const SignalWatcher& s) = delete;
	~SignalWatcher();

private:
	std::array<std::atomic_flag, (watcherEventsType_t)WatcherEvents::_SIZE> flags;


public:
	/** @brief Returns if the signal was caught. The flag ressets after the call.
	 *
	 *  @p index Which event's flag must be checked.
	 */
	bool checkAndClearFlag(WatcherEvents index);

private:
	static void notifyWatchers(WatcherEvents flag);
	static void callback(int signal);
};
