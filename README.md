## Overview
MatchManager is a command line tool that allows for multithreaded match execution between two UCI-compatible, allowing for precise ELO difference estimation. The system features parallelized IPC, time-controlled games, and randomization to ensure unbiased results. It communicates with each engine through stdin/stdout pipes, interprets UCI move outputs, and tracks game outcomes including wins, losses, draws, and undefined behavior such as crashes and invalid responses.

## Running MatchManager (command prompt)
```
MatchManager <engine1> <engine2> [-T <time>] [-t <threads>] [-f <fenfile>]
```
for example:

```
MatchManager tt64 tt128
// pits "tt64.exe" against "tt128.exe" using the default time controls, threads, and fenfile (100ms per move, 1 thread, lc01k.txt)

MatchManager tt64 tt128 -funfair.txt -t16 -T250
// does the same, but uses 250ms per move, draws fens from "unfair.txt", and runs the match 16 times in parallel
```

## Commands

Pause all matches:
```
pause
```

Resume all matches:
```
go
```

Stop all matches (and display all data gathered thus far):
```
stop
```
