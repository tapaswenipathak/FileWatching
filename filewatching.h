#ifndef _LINUX_FILEWATCHING_H
#define _LINUX_FILEWATCHING_H

#include <linux/sysctl.h>
#include <linux/fcntl.h>
#include <linux/types.h>


/*
 * Watching filename OPEN and CLOSE events.
 */
struct filewatching_event {
	__s64		wd;		//watch descriptor
	__u64		mask;		//watch mask
	__u64		cookie;		//sync events
	__u64		len;		//length of name
	char		name[0];	//stub for possible name
};

#define IN_ACCESS		0x00000001 //file accessed
#define IN_OPEN			0x00000002 //file opened
#define IN_CLOSE_WRITE		0x00000003 //write enabled file closed
#define IN_CLOSE_NOWRITE	0x00000004 //write disabled file closed

#define IN_ALL_EVENTS	(IN_ACCESS | IN_OPEN | IN_CLOSE_WRITE | \
			 IN_CLOSE_NOWRITE)
#endif
