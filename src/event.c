/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   event.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:31:28 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:31:28 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* A tiny "event" built on a mutex + cond + flag. The flag means a wake-up
** sent while the coder was not yet waiting is remembered instead of lost,
** which is what makes the check-then-wait pattern in acquire.c safe. */
void	coder_notify(t_coder *coder)
{
	pthread_mutex_lock(&coder->lock);
	coder->notified = 1;
	pthread_cond_signal(&coder->cond);
	pthread_mutex_unlock(&coder->lock);
}

/* Blocks until notified, or until the absolute time until_us (in us)
** when it is >= 0. Spurious wake-ups just loop. */
void	coder_wait(t_coder *coder, long until_us)
{
	struct timespec	ts;

	pthread_mutex_lock(&coder->lock);
	while (!coder->notified)
	{
		if (until_us < 0)
			pthread_cond_wait(&coder->cond, &coder->lock);
		else
		{
			us_to_timespec(until_us, &ts);
			if (pthread_cond_timedwait(&coder->cond, &coder->lock, &ts) != 0)
				break ;
		}
	}
	coder->notified = 0;
	pthread_mutex_unlock(&coder->lock);
}

/* Dongle i (0-based) sits between coder i - 1 and coder i. A change on
** it can alter the decision of those two and of their other neighbours
** (a blocked head two hops away may now yield), hence a window of 4. */
void	notify_window(t_sim *sim, int dongle_idx)
{
	int	n;
	int	k;

	n = sim->args.n_coders;
	k = -2;
	while (k <= 1)
	{
		coder_notify(&sim->coders[(dongle_idx + k + n + n) % n]);
		k++;
	}
}

void	notify_all_coders(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->args.n_coders)
	{
		coder_notify(&sim->coders[i]);
		i++;
	}
}
