/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:26:27 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	run_simulation(t_sim *sim)
{
	if (start_threads(sim) != 0)
		return (1);
	join_threads(sim);
	return (0);
}

int	main(int argc, char **argv)
{
	t_sim	sim;
	int		ret;

	memset(&sim, 0, sizeof(t_sim));
	if (parse_args(argc, argv, &sim.args) != 0)
		return (1);
	if (init_sim(&sim) != 0)
		return (1);
	ret = run_simulation(&sim);
	cleanup_sim(&sim);
	return (ret);
}
