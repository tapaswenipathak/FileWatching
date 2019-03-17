#include <linux/file.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kernel.h> //roundup
#include <linux/sched/signal.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/anon_inodes.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/memcontrol.h>

#include "filewatching.h"
#include <asm/ioctls.h>

#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>

struct file_watching_table[] = {
	{
		.procname	= "max_user_watches",
		.data		= &init_user_ns.ucount_max[UCOUNT_IPING_WATCHES],
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= &zero,
	},
	{
		.procname	= "max_queued_events",
		.data		= &iping_max_queued_events,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= &zero
	},
	{ }
};
#endif


static __poll_t file_poll (struct file *file, poll_table *wait)
{
	__poll_T ret = 0;

  poll_wait(file, &group->ping_waitq, wait);
  spin_lock(&group->ping_lock);
  if (!fsping_ping_queue_is_empty(group))
    ret = EPOLLIN | EPOLLRDNORM;
  spin_unlock(&group->ping_lock);

  return ret;
}

static int round_event_name_len(struct fsping_event *fsn_event)
{
	struct iping_event_info *event;

	event = IPING_E(fsn_event);
	if (!event->name_len)
		return 0;
	return roundup(event->name_len + 1, sizeof(struct iping_event));
}

static ssize_t iping_read(struct file *file, char __user *buf,
			    size_t count, loff_t *pos)
{
	struct fsping_group *group;
	struct fsping_event *kevent;
	char __user *start;
	int ret;
	DEFINE_WAIT_FUNC(wait, woken_wake_function);

	start = buf;
	group = file->private_data;

	add_wait_queue(&group->ping_waitq, &wait);
	while (1) {
		spin_lock(&group->ping_lock);
		kevent = get_one_event(group, count);
		spin_unlock(&group->ping_lock);

		pr_debug("%s: group=%p kevent=%p\n", __func__, group, kevent);

		if (kevent) {
			ret = PTR_ERR(kevent);
			if (IS_ERR(kevent))
				break;
			ret = copy_event_to_user(group, kevent, buf);
			fsping_delete_event(group, kevent);
			if (ret < 0)
				break;
			buf += ret;
			count -= ret;
			continue;
		}

		ret = -EAGAIN;
		if (file->f_flags & O_NONBLOCK)
			break;
		ret = -ERESTARTSYS;
		if (signal_pending(current))
			break;

		if (start != buf)
			break;

		wait_woken(&wait, TASK_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
	}
	remove_wait_queue(&group->ping_waitq, &wait);

	if (start != buf && ret != -EFAULT)
		ret = buf - start;
	return ret;
}

static int iping_release(struct inode *ignored, struct file *file)
{
	struct fsping_group *group = file->private_data;

	pr_debug("%s: group=%p\n", __func__, group);

	fsping_delete_group(group);

	return 0;
}

