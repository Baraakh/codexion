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

/* Both "has taken a dongle" lines and "is compiling" are printed under
** a single print_lock hold, so no other coder's line can land between a
** coder's acquisition and the compile it enables. */
void	log_take_and_compile(t_coder *coder)
{
	t_sim	*sim;
	long	ts;

	sim = coder->sim;
	pthread_mutex_lock(&sim->print_lock);
	if (!sim->stopped)
	{
		ts = elapsed_ms(sim);
		printf("%ld %d has taken a dongle\n", ts, coder->id);
		printf("%ld %d has taken a dongle\n", ts, coder->id);
		printf("%ld %d is compiling\n", ts, coder->id);
	}
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
