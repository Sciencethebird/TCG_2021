# 2048-Framework

Framework for 2048 & 2048-like Games (C++ 11)

## Basic Usage

To make the sample program:
```bash
make # see makefile for details
```

To run the sample program:
```bash
./2048 # by default the program runs 1000 games
```

To specify the total games to run:
```bash
./2048 --total=100000
```

To display the statistic every 1000 episodes:
```bash
./2048 --total=100000 --block=1000 --limit=1000
```

To specify the total games to run, and seed the environment:
```bash
./2048 --total=100000 --evil="seed=12345" # need to inherit from random_agent
```

To save the statistic result to a file:
```bash
./2048 --save=stat.txt
```

To load and review the statistic result from a file:
```bash
./2048 --load=stat.txt
```

## Judge

TCG 2021 Project 1 Judger V20210927
The provided judger, 2584-judge, can only be executed in the Linux environment.

Before running the judger, make sure that it remains executable:
```bash
chmod +x 2584-judge
```

To judge a statistic file "stat.txt":
```bash
./2584-judge --load=stat.txt --check
```

To judge a statistic file "stat.txt" under a given speed limit:
```bash
./2584-judge --load=stat.txt --check --judge="speed-threshold=100000"
```
To judge (One-liner)
```bash
cp stat.txt ../pj-1-judge-v1/stat.txt && ../pj-1-judge-v1/2584-judge --load=stat.txt --check
```
## Author

[Computer Games and Intelligence (CGI) Lab](https://cgilab.nctu.edu.tw/), NYCU, Taiwan
