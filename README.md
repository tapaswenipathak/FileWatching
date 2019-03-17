File Watching
-------------

A read or write watchpoint can be put in a file. As an subject 'open' operation
ping will happen. This can help watch any security breaches and monitoring activities
on a sensitive file.

This project implements a system call to plug/unplug a watchpoint and a command allowing
the user to invoke a system call.

The implementation is in VFS with the restriction that ping is per the return of the
system call that set the watchpoint and that such return implicitly releases the
watchpoint.

You need to add the system files, increment the sys call counter in .tbl file, install
the kernel and open the separate kernel.
