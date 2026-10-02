/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/17 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/10/02 16:18:38 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* print_lock guards both stdout and the stopped flag, so the burnout
** message and the stop decision are one atomic step: no coder can print
** after "burned out". */
int	sim_is_stopped(t_sim *sim)
{
	int	stopped;

	pthread_mutex_lock(&sim->print_lock);
	stopped = sim->stopped;
	pthread_mutex_unlock(&sim->print_lock);
	return (stopped);
}

void	set_stopped(t_sim *sim)
{
	pthread_mutex_lock(&sim->print_lock);
	sim->stopped = 1;
	pthread_mutex_unlock(&sim->print_lock);
}

void	log_state(t_coder *coder, const char *state)
{
	t_sim	*sim;

	sim = coder->sim;
	pthread_mutex_lock(&sim->print_lock);
	if (!sim->stopped)
		printf("%ld %d %s\n", elapsed_ms(sim), coder->id, state);
	pthread_mutex_unlock(&sim->print_lock);
}

void	log_burnout(t_coder *coder)
{
	t_sim	*sim;

	sim = coder->sim;
	pthread_mutex_lock(&sim->print_lock);
	if (!sim->stopped)
	{
		printf("%ld %d burned out\n", elapsed_ms(sim), coder->id);
		sim->stopped = 1;
	}
	pthread_mutex_unlock(&sim->print_lock);
}
