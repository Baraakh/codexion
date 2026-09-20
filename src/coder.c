/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:31:28 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:31:28 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	wait_start(t_sim *sim)
{
	pthread_mutex_lock(&sim->start_lock);
	while (!sim->started)
		pthread_cond_wait(&sim->start_cond, &sim->start_lock);
	pthread_mutex_unlock(&sim->start_lock);
}

/* A lone coder owns a single dongle: they take it once and can only
** wait for the monitor to end the simulation. */
static int	take_dongles(t_coder *coder)
{
	if (!dongle_acquire_pair(coder))
		return (0);
	log_state(coder, "has taken a dongle");
	if (coder->first == coder->second)
	{
		while (!sim_is_stopped(coder->sim))
			usleep(SLEEP_STEP_US);
		dongle_release(coder->first, coder);
		return (0);
	}
	log_state(coder, "has taken a dongle");
	return (1);
}

/* Deadline is refreshed before the log line so the monitor never sees a
** stale value for a coder that is already compiling. Symmetrically,
** "is debugging" is logged before the dongles go back on the table so
** no neighbour can log "is compiling" with them while this coder still
** reads as compiling in the log. */
static void	start_compiling(t_coder *coder)
{
	pthread_mutex_lock(&coder->lock);
	coder->last_start = get_time_us();
	coder->count++;
	pthread_mutex_unlock(&coder->lock);
	log_state(coder, "is compiling");
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	t_args	*args;

	coder = (t_coder *)arg;
	args = &coder->sim->args;
	wait_start(coder->sim);
	while (!sim_is_stopped(coder->sim))
	{
		if (!take_dongles(coder))
			break ;
		start_compiling(coder);
		smart_sleep(coder->sim, args->t_compile);
		log_state(coder, "is debugging");
		dongle_release(coder->first, coder);
		dongle_release(coder->second, coder);
		smart_sleep(coder->sim, args->t_debug);
		log_state(coder, "is refactoring");
		smart_sleep(coder->sim, args->t_refactor);
	}
	return (NULL);
}
