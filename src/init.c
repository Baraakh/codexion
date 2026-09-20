/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:26:27 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* sync_ready counts how many of the four primitives exist, so cleanup
** can destroy exactly those after a failure. */
static int	init_sync(t_sim *sim)
{
	if (pthread_mutex_init(&sim->print_lock, NULL) != 0)
		return (1);
	sim->sync_ready = 1;
	if (pthread_mutex_init(&sim->table_lock, NULL) != 0)
		return (1);
	sim->sync_ready = 2;
	if (pthread_mutex_init(&sim->start_lock, NULL) != 0)
		return (1);
	sim->sync_ready = 3;
	if (pthread_cond_init(&sim->start_cond, NULL) != 0)
		return (1);
	sim->sync_ready = 4;
	return (0);
}

static int	init_dongles(t_sim *sim)
{
	int			i;
	t_dongle	*d;

	i = 0;
	while (i < sim->args.n_coders)
	{
		d = &sim->dongles[i];
		d->id = i + 1;
		if (heap_init(&d->queue, DONGLE_QUEUE_CAP) != 0)
			return (1);
		if (pthread_mutex_init(&d->mutex, NULL) != 0)
			return (1);
		sim->dongles_ready++;
		i++;
	}
	return (0);
}

/* Coder i (0-based) sits between dongle i and dongle (i + 1) % n.
** first/second are ordered by index so every coder locks the lower
** dongle first. With one coder both pointers are the same dongle. */
static void	assign_dongles(t_coder *coder, t_sim *sim, int i)
{
	int	left;
	int	right;

	left = i;
	right = (i + 1) % sim->args.n_coders;
	if (right < left)
	{
		coder->first = &sim->dongles[right];
		coder->second = &sim->dongles[left];
	}
	else
	{
		coder->first = &sim->dongles[left];
		coder->second = &sim->dongles[right];
	}
}

static int	init_coders(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->args.n_coders)
	{
		sim->coders[i].id = i + 1;
		sim->coders[i].sim = sim;
		assign_dongles(&sim->coders[i], sim, i);
		if (pthread_mutex_init(&sim->coders[i].lock, NULL) != 0)
			return (1);
		if (pthread_cond_init(&sim->coders[i].cond, NULL) != 0)
		{
			pthread_mutex_destroy(&sim->coders[i].lock);
			return (1);
		}
		sim->coders_ready++;
		i++;
	}
	return (0);
}

int	init_sim(t_sim *sim)
{
	int	n;

	n = sim->args.n_coders;
	sim->coders = malloc(sizeof(t_coder) * n);
	sim->dongles = malloc(sizeof(t_dongle) * n);
	if (sim->coders != NULL && sim->dongles != NULL)
	{
		memset(sim->coders, 0, sizeof(t_coder) * n);
		memset(sim->dongles, 0, sizeof(t_dongle) * n);
		if (init_sync(sim) == 0 && init_dongles(sim) == 0
			&& init_coders(sim) == 0)
			return (0);
	}
	fprintf(stderr, "Error: initialization failed\n");
	cleanup_sim(sim);
	return (1);
}
