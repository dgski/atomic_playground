# Atomic Playground

Strengthening my atomic operation knowledge with some hands-on coding.

## Components
- [WaitFreeChannel](./WaitFreeChannel/WaitFreeChannel.hpp): Inter-thread communication 'channel' with lock and Wait free reading, lock-free writing. Results on M1 Macbook Air:
    - locking averagePollingTimeNs=652
    - waitFree averagePollingTimeNs=51
- [WaitFreeQueue](./WaitFreeQueue/WaitFreeQueue.hpp): Inter-thread queue with lock and wait free reading, lock-free writing. Results on M1 Macbook Air:
    - lockingTotalThroughputPerSecond=7128056
    - waitFreeTotalThroughputPerSecond=15051376