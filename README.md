# Atomic Playground

Strengthening my atomic operation knowledge with some hands-on coding.

## Components
- [WaitFreeChannel](./WaitFreeChannel/WaitFreeChannel.hpp): Inter-thread communication 'channel' with lock and Wait free reading, lock-free writing. Results on M1 Macbook Air:
    - locking averagePollingTime=1167
    - waitFree averagePollingTime=39