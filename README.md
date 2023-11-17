# Atomic Playground

Strengthening my atomic operation knowledge with some hands-on coding.

## Components
- [WaitFreeChannel](./WaitFreeChannel/WaitFreeChannel.hpp): Inter-thread communication 'channel' with lock and Wait free reading, lock-free writing. Results on M1 Macbook Air:
    - locking averagePollingTimeNs=652
    - waitFree averagePollingTimeNs=51
- [WaitFreeQueue](./WaitFreeQueue/WaitFreeQueue.hpp): Inter-thread queue with lock and wait free reading, lock-free writing. Results on M1 Macbook Air:
    - lockingTotalThroughputPerSecond=7128056
    - waitFreeTotalThroughputPerSecond=15051376
- [WaitFreeAsyncWorker](./WaitFreeAsyncWorker/WaitFreeAsyncWorker.hpp): Single background thread worker that leverages wait-free queues to allow rapid scheduling of tasks and subsequent callbacks. Results on M1 Macbook Air:
    - averageSchedulingTimeNs=97
    - averagePollingTimeNs=67
- [WaitFreeChannelSingleUse](./WaitFreeChannelSingleUse/WaitFreeChannelSingleUse.hpp): Inter-thread communication 'channel' with lock and Wait free reading, lock-free writing. Can use more simpler, more relaxed atomic operations as there is no intention of re-use.  Results on M1 Macbook Air:
    - locking averagePollingTimeNs=readTimeTaken=46ns, writeTimeTaken=41ns
    - waitFree averagePollingTimeNs=readTimeTaken=24ns, writeTimeTaken=39ns