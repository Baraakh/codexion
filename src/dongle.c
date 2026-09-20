/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:36:37 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:36:37 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* Every dongle field is read or written only under d->mutex. */
void	dongle_enqueue(t_dongle *d, t_request req)
{
	pthread_mutex_lock(&d->mutex);
	heap_push(&d->queue, req);
	pthread_mutex_unlock(&d->mutex);
}

int	dongle_is_free(t_dongle *d, long *available_at)
{
	int	is_free;

	pthread_mutex_lock(&d->mutex);
	is_free = (d->holder == 0);
	if (d->available_at > *available_at)
		*available_at = d->available_at;
	pthread_mutex_unlock(&d->mutex);
	return (is_free);
}

/* Id of the highest-priority requester, 0 if nobody is queued. */
int	dongle_head(t_dongle *d)
{
	t_request	*top;
	int			id;

	pthread_mutex_lock(&d->mutex);
	id = 0;
	top = heap_peek(&d->queue);
	if (top != NULL)
		id = top->coder_id;
	pthread_mutex_unlock(&d->mutex);
	return (id);
}

void	dongle_take(t_dongle *d, t_coder *coder)
{
	pthread_mutex_lock(&d->mutex);
	heap_remove(&d->queue, coder->id);
	d->holder = coder->id;
	pthread_mutex_unlock(&d->mutex);
}

/* Starts the cooldown, then wakes every coder whose decision may change. */
void	dongle_release(t_dongle *d, t_coder *coder)
{
	pthread_mutex_lock(&d->mutex);
	d->holder = 0;
	d->available_at = get_time_us() + coder->sim->args.cooldown;
	pthread_mutex_unlock(&d->mutex);
	notify_window(coder->sim, d->id - 1);
}
