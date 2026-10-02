/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/10/02 16:18:31 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long	get_time_us(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)tv.tv_sec * 1000000L + tv.tv_usec);
}

long	elapsed_ms(t_sim *sim)
{
	return ((get_time_us() - sim->start_us) / 1000);
}

/* Sleeps in small steps so we neither oversleep nor ignore a stop. */
void	smart_sleep(t_sim *sim, long us)
{
	long	target;
	long	remaining;

	target = get_time_us() + us;
	remaining = us;
	while (remaining > 0)
	{
		if (sim_is_stopped(sim))
			return ;
		if (remaining > SLEEP_STEP_US)
			usleep(SLEEP_STEP_US);
		else
			usleep(remaining);
		remaining = target - get_time_us();
	}
}

/* Absolute CLOCK_REALTIME timespec for pthread_cond_timedwait. */
void	us_to_timespec(long abs_us, struct timespec *ts)
{
	ts->tv_sec = abs_us / 1000000L;
	ts->tv_nsec = (abs_us % 1000000L) * 1000L;
}
