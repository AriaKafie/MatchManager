## Overview
MatchManager is a command line tool that allows for multithreaded match execution between two UCI-compatible chess engines, allowing for precise ELO difference estimation. The system features parallelized IPC, time-controlled games, and randomization to ensure unbiased results. It communicates with each engine through stdin/stdout pipes, interprets UCI move outputs, and tracks game outcomes including wins, losses, draws, and undefined behavior such as crashes and invalid responses.

## Running MatchManager (command prompt)
```
MatchManager --engine1=path\to\engine1.exe --engine2=path\to\engine2.exe
// pits engine1.exe against engine2.exe using the default time controls, threads, and fen file (100ms per move, 1 thread, lc01k.txt)

MatchManager --time=250 --engine1=path\to\engine1.exe --engine2=path\to\engine2.exe --fen_file=path\to\fens.txt --threads=16
// does the same, but runs the match 16 times in parallel, with custom time and fen file settings
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
