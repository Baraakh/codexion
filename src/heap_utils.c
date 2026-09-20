/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:26:27 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/* Strict ordering: smaller key first, then lower coder id (EDF tie-break). */
int	request_less(t_heap *h, int a, int b)
{
	t_request	*ra;
	t_request	*rb;

	ra = &h->data[a];
	rb = &h->data[b];
	if (ra->key != rb->key)
		return (ra->key < rb->key);
	return (ra->coder_id < rb->coder_id);
}

void	swap_requests(t_request *a, t_request *b)
{
	t_request	tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void	sift_up(t_heap *h, int i)
{
	int	parent;

	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (!request_less(h, i, parent))
			return ;
		swap_requests(&h->data[i], &h->data[parent]);
		i = parent;
	}
}

void	sift_down(t_heap *h, int i)
{
	int	left;
	int	smallest;

	while (1)
	{
		left = 2 * i + 1;
		smallest = i;
		if (left < h->size && request_less(h, left, smallest))
			smallest = left;
		if (left + 1 < h->size && request_less(h, left + 1, smallest))
			smallest = left + 1;
		if (smallest == i)
			return ;
		swap_requests(&h->data[i], &h->data[smallest]);
		i = smallest;
	}
}
