#include <linux/cpuidle.h>
#include <linux/module.h>
#include <linux/smp.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>

/* Wait time before enabling single-core mode after boot (e.g., 60 seconds) */
#define SINGLE_CORE_MODE_DELAY (60 * HZ)

static int null_enable_device(struct cpuidle_driver *drv,
			      struct cpuidle_device *dev)
{
	return 0;
}

static int null_select(struct cpuidle_driver *drv, struct cpuidle_device *dev)
{
	/* Allow all cores to idle normally during the delay period */
	if (time_after(jiffies, SINGLE_CORE_MODE_DELAY)) {
		/* Only allow CPU 0 to stay active, put others offline if possible */
		if (dev->cpu != 0) {
			cpu_down(dev->cpu);
			return -1; /* Indicate no idle state for other cores */
		}
	} else {
		/* Allow normal idling behavior for all cores */
		return CPUIDLE_DRIVER_STATE_START;
	}
}

static struct cpuidle_governor null_governor = {
	.name =		"null",
	.rating =	5,
	.enable =	null_enable_device,
	.select =	null_select,
	.owner =	THIS_MODULE,
};

static int __init init_null(void)
{
	return cpuidle_register_governor(&null_governor);
}

/* Register with postcore_init to defer initialization until core kernel setup completes */
postcore_initcall(init_null);
