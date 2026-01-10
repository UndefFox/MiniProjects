# Implementation

The implementation is simple: a ParticleEngine that generates a simple snowfall animation. To handle leaving the program and dynamically updating the resolution, a SignalWatcher class was introduced to provide a simple way for receiving and handling signals. It's based on a global subscription list that is iterated over on signal arrival. The list is specifically designed to always stay in an iterable state when accessed from a single thread at once.

### Points of interest

- src/signalwatcher.h : SignalWatcher class - Handling signals with buffered flags.
- src/forwardlist.hpp : ForwardList class - Interruption-safe list.
- src/main.cpp : processInputs function - Proper handling of input parameters.
