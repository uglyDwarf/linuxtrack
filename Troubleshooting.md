# Introduction #

Here are few pointers that should help people experiencing difficulties using linuxtrack.

  * Run XPlane from terminal
  * When segfault occurs, try to get gdb backtrace
  * Examine log.txt
  * Don't be afraid to ask for help

# Details #

## Run XPlane from terminal ##
Although it scares some people, there is not much about it...

Just run your favorite terminal and run XPlane from it - the reason is, that terminal tells you many things that your desktop doesn't. For example when XPlane window closes down suddenly, terminal might say something like "Segmentation fault" and it will give you hint on what is going on.

Also when you experience intermittent tracking, in terminal you'd see messages saying there is a problem tracking your model (LEDs too dim, you are outside of trackers field of view and so on)...

## When segfault occurs, try to get gdb backtrace ##
Segfault is not a very nice thing to happen, but when it does, it can help in the end...

So if you run XPlane and it ends up with segfault, run the XPlane from terminal, but not by itself, but in gdb.

```
cd /mount/stuff/XPlane/X-Plane 8.40
gdb ./X-Plane-i586
run
```

At this point, XPlane starts and you should try to reproduce the segfault - try to do he things you did when segfault occured and hope it will crash again.

Sometimes it does, sometimes it doesn't...

If it does, go to the terminal and type in this:

```
set pagination off
thread apply all backtrace
```

This makes gdb to output info on where the problem occured. In case you want to be a little adventurous, you can examine it by yourself, otherwise you can contact me and I'll try to fix the problem.

## Examine log.txt ##
Linuxtrack creates file named 'log,txt' and it should contain info useful for debugging problems.

It is a little bit confusing that XPlane creates file 'Log.txt' (note the uppercase L!).

Both files reside in the same directory the XPlane binary does.

When tracker doesn't work the way it should, be sure to check log.txt to see if it doesn't contain any useful info.

## Don't be afraid to ask ##
If you don't know what to do, feel free to ask... We won't laugh at you, even if the question would look stupid - maybe there are other people who experience the same difficulty and the answer would help them too...

You can fill in an issue here, you can chime in on XPlane.org forum and we'll try to help.

Here are things that would help pinpointing the problem:

  * Describe your problem as clearly as possible - "It doesn't work" is not helping...
  * If the crash occured and you managed to reproduce it, please attach gdb backtrace
  * Attach also log.txt file (in the same dir as XPlane binary) and your .linuxtrack file (in your home directory)