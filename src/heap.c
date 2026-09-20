/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:26:27 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	heap_init(t_heap *h, int cap)
{
	h->size = 0;
	h->cap = cap;
	h->data = malloc(sizeof(t_request) * cap);
	if (h->data == NULL)
		return (1);
	memset(h->data, 0, sizeof(t_request) * cap);
	return (0);
}

void	heap_free(t_heap *h)
{
	free(h->data);
	h->data = NULL;
	h->size = 0;
	h->cap = 0;
}

int	heap_push(t_heap *h, t_request req)
{
	if (h->size >= h->cap)
		return (1);
	h->data[h->size] = req;
	h->size++;
	sift_up(h, h->size - 1);
	return (0);
}

/* Removes the request of one coder wherever it sits in the heap:
** swap with the last element, then restore the invariant both ways. */
int	heap_remove(t_heap *h, int coder_id)
{
	int	i;

	i = 0;
	while (i < h->size && h->data[i].coder_id != coder_id)
		i++;
	if (i == h->size)
		return (1);
	h->size--;
	if (i < h->size)
	{
		h->data[i] = h->data[h->size];
		sift_down(h, i);
		sift_up(h, i);
	}
	return (0);
}

t_request	*heap_peek(t_heap *h)
{
	if (h->size == 0)
		return (NULL);
	return (&h->data[0]);
}
