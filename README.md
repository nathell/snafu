# snafu

Have you ever wondered what fraction of your work time is actually devoted to work, or how often you turn on your email program? Now you can find out. Activities like checking mail or reading news can be very addictive, so I wrote a little utility that keeps track of what window is currently active, and upon termination generates a report containing these data. It’s called _snafu_, which, apart from its usual meaning, for purposes of this little project also stands for “Superb Nathell’s Addiction Fighting Utility”.

Snafu is written in C++ and is available as free software under the terms of MIT license. Currently, it supports only X, so it won't be of much use for Windows users. I've tested it under Kubuntu 7.10, which has X.org 7.3 and GCC 4.1.3, but any Unix-like system with X and a decent C++ compiler should do. (_Addendum 2016:_ It still works on a Linux Mint 17, after a minor Makefile tweak.) As usual, however, YMMV.

## How to use it?

- Download and compile it. More on that below.
- Run the binary snafu, possibly redirecting its standard output if you want the results to go to a file.
- Do some work. After a while, send it a `SIGQUIT` signal. This can be done by pressing `Ctrl+\` on the terminal running snafu, or saying `killall -QUIT snafu` in a Bourne shell.
- Snafu will output a report that looks like this:

```
Statistics of window frequency:
 95,803% --- emacs@chamsin
  4,015% --- xterm
  0,182% --- Kadu

Window changed every 13,700 seconds
```

## Compilation

Just say `make`. This should produce a `snafu` binary which you can install to a directory of your choice.

## Comments, feedback, patches

If anyone finds this tool useful, I'd be happy to know it. Please feel free to send email to dj at danieljanus dot pl.
