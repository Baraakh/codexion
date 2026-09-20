/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:26:27 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	check_coder(t_coder *coder, int *all_done)
{
	long	now;
	int		burned;

	now = get_time_us();
	pthread_mutex_lock(&coder->lock);
	burned = (now - coder->last_start > coder->sim->args.t_burnout);
	if (coder->count < coder->sim->args.compiles_req)
		*all_done = 0;
	pthread_mutex_unlock(&coder->lock);
	return (burned);
}

/* Returns 1 once the simulation must stop (burnout or everyone done). */
static int	check_all(t_sim *sim)
{
	int	i;
	int	all_done;

	all_done = 1;
	i = 0;
	while (i < sim->args.n_coders)
	{
		if (check_coder(&sim->coders[i], &all_done))
		{
			log_burnout(&sim->coders[i]);
			return (1);
		}
		i++;
	}
	if (all_done)
	{
		set_stopped(sim);
		return (1);
	}
	return (0);
}

void	*monitor_routine(void *arg)
{
	t_sim	*sim;

	sim = (t_sim *)arg;
	while (!check_all(sim))
		usleep(MONITOR_STEP_US);
	notify_all_coders(sim);
	return (NULL);
}
