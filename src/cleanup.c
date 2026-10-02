/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/14 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/10/02 16:17:55 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	destroy_dongles(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->dongles_ready)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		i++;
	}
	i = 0;
	while (i < sim->args.n_coders)
	{
		heap_free(&sim->dongles[i].queue);
		i++;
	}
}

static void	destroy_coders(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->coders_ready)
	{
		pthread_mutex_destroy(&sim->coders[i].lock);
		pthread_cond_destroy(&sim->coders[i].cond);
		i++;
	}
}

/* Safe to call after a partial init: only what was set up is torn down. */
void	cleanup_sim(t_sim *sim)
{
	if (sim->dongles != NULL)
		destroy_dongles(sim);
	if (sim->coders != NULL)
		destroy_coders(sim);
	if (sim->sync_ready >= 1)
		pthread_mutex_destroy(&sim->print_lock);
	if (sim->sync_ready >= 2)
		pthread_mutex_destroy(&sim->table_lock);
	if (sim->sync_ready >= 3)
		pthread_mutex_destroy(&sim->start_lock);
	if (sim->sync_ready >= 4)
		pthread_cond_destroy(&sim->start_cond);
	free(sim->dongles);
	free(sim->coders);
	sim->dongles = NULL;
	sim->coders = NULL;
}
