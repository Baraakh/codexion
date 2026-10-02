/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   threads.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/13 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/10/02 16:17:31 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* All coder threads block on the start gate, so they share a common
** t = 0 and an identical initial deadline. */
static void	open_start_gate(t_sim *sim)
{
	int		i;
	long	now;

	now = get_time_us();
	i = 0;
	while (i < sim->args.n_coders)
	{
		pthread_mutex_lock(&sim->coders[i].lock);
		sim->coders[i].last_start = now;
		pthread_mutex_unlock(&sim->coders[i].lock);
		i++;
	}
	pthread_mutex_lock(&sim->start_lock);
	sim->start_us = now;
	sim->started = 1;
	pthread_cond_broadcast(&sim->start_cond);
	pthread_mutex_unlock(&sim->start_lock);
}

static void	join_coders(t_sim *sim, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
}

static int	abort_start(t_sim *sim, int created)
{
	set_stopped(sim);
	if (!sim->started)
		open_start_gate(sim);
	notify_all_coders(sim);
	join_coders(sim, created);
	fprintf(stderr, "Error: pthread_create failed\n");
	return (1);
}

int	start_threads(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->args.n_coders)
	{
		if (pthread_create(&sim->coders[i].thread, NULL,
				coder_routine, &sim->coders[i]) != 0)
			return (abort_start(sim, i));
		i++;
	}
	open_start_gate(sim);
	if (pthread_create(&sim->monitor, NULL, monitor_routine, sim) != 0)
		return (abort_start(sim, i));
	return (0);
}

void	join_threads(t_sim *sim)
{
	join_coders(sim, sim->args.n_coders);
	pthread_join(sim->monitor, NULL);
}
