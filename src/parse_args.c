/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_args.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/11 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/10/02 16:16:29 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	print_error(const char *msg)
{
	fprintf(stderr, "Error: %s\n", msg);
	return (1);
}

/* Accepts only a non-empty string of ASCII digits that fits in an int. */
static int	parse_positive(const char *s, long *out)
{
	long	val;
	int		i;

	if (s[0] == '\0')
		return (1);
	val = 0;
	i = 0;
	while (s[i] != '\0')
	{
		if (s[i] < '0' || s[i] > '9')
			return (1);
		val = val * 10 + (s[i] - '0');
		if (val > INT_MAX)
			return (1);
		i++;
	}
	*out = val;
	return (0);
}

static int	parse_scheduler(const char *s, t_sched *out)
{
	if (strcmp(s, "fifo") == 0)
		*out = POLICY_FIFO;
	else if (strcmp(s, "edf") == 0)
		*out = POLICY_EDF;
	else
		return (1);
	return (0);
}

/* Durations are given in ms on the command line, stored in us. */
static int	parse_numbers(char **argv, t_args *args)
{
	long	vals[7];
	int		i;

	i = 0;
	while (i < 7)
	{
		if (parse_positive(argv[i + 1], &vals[i]) != 0)
			return (1);
		i++;
	}
	args->n_coders = (int)vals[0];
	args->t_burnout = vals[1] * 1000;
	args->t_compile = vals[2] * 1000;
	args->t_debug = vals[3] * 1000;
	args->t_refactor = vals[4] * 1000;
	args->compiles_req = (int)vals[5];
	args->cooldown = vals[6] * 1000;
	if (args->n_coders < 1)
		return (1);
	return (0);
}

int	parse_args(int argc, char **argv, t_args *args)
{
	if (argc != 9)
		return (print_error("usage: ./codexion number_of_coders "
				"time_to_burnout time_to_compile time_to_debug "
				"time_to_refactor number_of_compiles_required "
				"dongle_cooldown scheduler(fifo|edf)"));
	if (parse_numbers(argv, args) != 0)
		return (print_error("numeric arguments must be non-negative "
				"integers within int range, number_of_coders >= 1"));
	if (parse_scheduler(argv[8], &args->sched) != 0)
		return (print_error("scheduler must be exactly 'fifo' or 'edf'"));
	return (0);
}
