/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   acquire.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/22 21:36:37 by bkhilo            #+#    #+#             */
/*   Updated: 2026/10/02 16:19:37 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* FIFO: arrival time. EDF: absolute deadline. Ties are broken by the
** heap comparator (lowest coder id), so the order is always total. */
static long	request_key(t_coder *coder)
{
	long	key;

	if (coder->sim->args.sched == POLICY_FIFO)
		return (get_time_us());
	pthread_mutex_lock(&coder->lock);
	key = coder->last_start + coder->sim->args.t_burnout;
	pthread_mutex_unlock(&coder->lock);
	return (key);
}

/* A competitor at dongle d "can go" when the other dongle of its pair
** is free (a running cooldown still counts as free: it will go soon). */
static int	competitor_can_go(t_coder *x, t_dongle *d)
{
	t_dongle	*other;
	long		unused;

	other = x->first;
	if (other == d)
		other = x->second;
	if (other == d)
		return (1);
	unused = 0;
	return (dongle_is_free(other, &unused));
}

/* Priority at d belongs to the queue head, unless that head is stuck
** waiting for its other dongle: then it yields so the ring does not
** collapse into a convoy behind one blocked coder. */
static int	has_priority(t_coder *coder, t_dongle *d)
{
	int	head;

	head = dongle_head(d);
	if (head == 0 || head == coder->id)
		return (1);
	return (!competitor_can_go(&coder->sim->coders[head - 1], d));
}

/* Returns 1 if both dongles were taken, 0 if the coder must wait for a
** state change, 2 if only a cooldown is pending (*ready_at says when).
** table_lock serializes decisions so two coders never both conclude
** they may take the same free dongle. */
static int	try_take(t_coder *coder, long *ready_at)
{
	int	res;

	res = 0;
	*ready_at = 0;
	pthread_mutex_lock(&coder->sim->table_lock);
	if (dongle_is_free(coder->first, ready_at)
		&& dongle_is_free(coder->second, ready_at)
		&& has_priority(coder, coder->first)
		&& has_priority(coder, coder->second))
	{
		res = 2;
		if (get_time_us() >= *ready_at)
		{
			dongle_take(coder->first, coder);
			if (coder->second != coder->first)
				dongle_take(coder->second, coder);
			res = 1;
		}
	}
	pthread_mutex_unlock(&coder->sim->table_lock);
	if (res == 1)
		notify_window(coder->sim, coder->first->id - 1);
	if (res == 1 && coder->second != coder->first)
		notify_window(coder->sim, coder->second->id - 1);
	return (res);
}

/* Take-both-or-none: a coder never holds one dongle while waiting for
** the other (no hold-and-wait). Between checks it sleeps on its own
** event; every dongle change notifies the coders around it, so no
** state change is missed. */
int	dongle_acquire_pair(t_coder *coder)
{
	t_request	req;
	long		ready_at;
	int			res;

	req.coder_id = coder->id;
	req.key = request_key(coder);
	dongle_enqueue(coder->first, req);
	if (coder->second != coder->first)
		dongle_enqueue(coder->second, req);
	while (!sim_is_stopped(coder->sim))
	{
		res = try_take(coder, &ready_at);
		if (res == 1)
			return (1);
		if (res == 2)
			coder_wait(coder, ready_at);
		else
			coder_wait(coder, -1);
	}
	return (0);
}
