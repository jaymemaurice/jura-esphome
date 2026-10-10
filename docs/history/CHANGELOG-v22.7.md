# v22.7

- Fixes batch aborts caused by `RR:F0` timing out while the Jura is physically grinding/brewing.
- A post-beverage `RR:F0` timeout during `WAIT_NOT_READY` or `WAIT_READY` is now an expected busy observation: it does not increment communication errors, mark communication failed, start recovery, or stop the batch.
- The first such timeout is accepted as evidence that the machine left Ready and entered its brew-busy period.
- Ready probing resumes after a 3 s retry delay; each unanswered probe still uses the configured command timeout, avoiding aggressive UART traffic while the machine is busy.
- The post-brew Ready-return window is increased to 5 minutes before the batch is considered stuck.
- Lifetime-counter verification remains mandatory before a product is counted complete or the next repetition starts.
- Keeps v22.6 explicit-operation preemption and v22.5 RAM status mappings unchanged.
