# whack-run

whack-run is a small binary used as part of [whack][]. To read more about why
whack-run is necessary to use whack, read the section
[How does Whack work?][how-does-whack-work]. 

[whack]: https://github.com/mwilliamson/whack
[how-does-whack-work]: https://github.com/mwilliamson/whack/blob/master/README.md#how-does-whack-work

## Installation

As a normal user:

```
make
```

As root:
```
make install
```

## Usage

whack-run is intended to be used with whack,
although you're welcome to use it for other purposes.
You can invoke whack-run like so:

```
whack-run &lt;apps-dir&gt; &lt;app&gt; &lt;args&gt;
```

Roughly speaking,
this mounts `&lt;apps-dir&gt;` to `/usr/local/whack` in a private mount namespace,
and then runs `&lt;app&gt;` with arguments `&lt;args&gt;`.
Since whack-run uses `unshare` and `mount`, whack-run has the `setuid` bit set.
It drops these privileges before invoking the specified application.

More precisely:

# `unshare(CLONE_NEWNS)` creates a private mount namespace.
  This means that any future `mount` calls in the process only affect that process.

# The directory `/usr/local/whack` is created if it doesn't already exist.

# Any existing mount at `/usr/local/whack` is unmounted.
  To see why not doing so could be problematic,
  consider if we run `script-parent` under `root-parent`,
  which then runs `script-child` under `root-child`.
  If `script-child` starts a long-running daemon,
  then we can't remove the directory `root-parent`
  since it contains the mount point for `root-child`.

# `setguid` privileges are dropped.

# `exec` is used to invoke the specified application.

## Example

The below is intended to show how whack-run works.
For an actual use case, take a look at whack.

```
$ mkdir -p example
$ echo -n 'Hello ' &gt; example/message
$ echo '#!/usr/bin/env sh' &gt; example/greet
$ echo 'cat /usr/local/whack/message' &gt;&gt; example/greet
$ echo 'echo $1' &gt;&gt; example/greet
$ chmod +x example/greet
$ whack-run example /usr/local/whack/greet Bob
Hello Bob
```
