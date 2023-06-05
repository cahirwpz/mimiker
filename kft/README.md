# Kernel Function Trace


## Design in C

KFT enables code instrumentation and provides functions that are called on each function
entry and exit.
For each event it stores the data about this event:
- type of event (entry or exit from function)
- thread id
- pc
- timestamp

To save space each entry is compressed into singe 64 bit value:
```
| 24 bits | 31 bits   |   8 bits  | 1 bit |
|    PC   | timestamp | thread id | event |
```


## Design in Python

Python utilities to analyze the data gathered during single mimiker run are divided into
two separate modules:

1. `preprocess-data.py` -- decode the data saved by kernel
2. `analayze-thread.py` -- analyze the data and print some readable statistics


### Preprocessing data

The data saved by the kernel is huge and contains information about all threads.
During preprocessing it is split into separate files that describe operation only of
single thread.
For every thread the timestamps are modified to show time that passed from thread
creation when the thread was running (when there was context switch it shouldn't be
counted to the thread time).

Data describing single thread is dumped into a file `thread-{id}.pkl`
(in [pickle](https://docs.python.org/3/library/pickle.html) format).

Additionally during preprocessing the `mimiker.elf` file is inspected to extract function
symbols.
It creates 2 dictionaries to translate functions to PC values and vice versa.
They are saved into files `pc2fun.pkl` and `fun2pc.pkl`.


### Analyzing data

To display statistics the second utility is used.
It takes a thread as argument.

Statistics that may be obtained:

- function call graph
- basic stats for functions (avg time, max time, invocation count)


## TODO
- [ ] Specify functions to analyze as arguments
- [ ] there are some functions that don't have exit (e.g. during context switch)
