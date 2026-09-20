/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bkhilo <bkhilo@student.42abudhabi.ae>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/16 21:26:27 by bkhilo            #+#    #+#             */
/*   Updated: 2026/09/16 21:26:27 by bkhilo           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

# include <pthread.h>
# include <sys/time.h>
# include <time.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <limits.h>

/* Every internal timestamp/duration is in microseconds; logs print ms. */
# define SLEEP_STEP_US		500
# define MONITOR_STEP_US	500
# define DONGLE_QUEUE_CAP	2

typedef enum e_sched
{
	POLICY_FIFO,
	POLICY_EDF
}	t_sched;

typedef struct s_args
{
	int			n_coders;
	long		t_burnout;
	long		t_compile;
	long		t_debug;
	long		t_refactor;
	int			compiles_req;
	long		cooldown;
	t_sched		sched;
}	t_args;

/* One pending request in a dongle's wait queue.
** key = arrival sequence number (fifo) or absolute deadline in us (edf). */
typedef struct s_request
{
	int		coder_id;
	long	key;
}	t_request;

/* Array-based binary min-heap of t_request (smallest key first,
** ties broken by lowest coder_id). */
typedef struct s_heap
{
	t_request	*data;
	int			size;
	int			cap;
}	t_heap;

/* holder/available_at/queue are protected by mutex. */
typedef struct s_dongle
{
	int				id;
	pthread_mutex_t	mutex;
	int				holder;
	long			available_at;
	t_heap			queue;
}	t_dongle;

/* lock protects last_start, count and the notified/cond event pair. */
typedef struct s_coder
{
	int				id;
	pthread_t		thread;
	pthread_mutex_t	lock;
	pthread_cond_t	cond;
	int				notified;
	long			last_start;
	int				count;
	t_dongle		*first;
	t_dongle		*second;
	struct s_sim	*sim;
}	t_coder;

typedef struct s_sim
{
	t_args			args;
	long			start_us;
	t_coder			*coders;
	t_dongle		*dongles;
	pthread_t		monitor;
	pthread_mutex_t	print_lock;
	int				stopped;
	pthread_mutex_t	table_lock;
	pthread_mutex_t	start_lock;
	pthread_cond_t	start_cond;
	int				started;
	int				sync_ready;
	int				dongles_ready;
	int				coders_ready;
}	t_sim;

/* parse_args.c */
int			parse_args(int argc, char **argv, t_args *args);

/* time.c */
long		get_time_us(void);
long		elapsed_ms(t_sim *sim);
void		smart_sleep(t_sim *sim, long us);
void		us_to_timespec(long abs_us, struct timespec *ts);

/* log.c */
int			sim_is_stopped(t_sim *sim);
void		set_stopped(t_sim *sim);
void		log_state(t_coder *coder, const char *state);
void		log_burnout(t_coder *coder);

/* heap.c / heap_utils.c */
int			heap_init(t_heap *h, int cap);
void		heap_free(t_heap *h);
int			heap_push(t_heap *h, t_request req);
int			heap_remove(t_heap *h, int coder_id);
t_request	*heap_peek(t_heap *h);
int			request_less(t_heap *h, int a, int b);
void		swap_requests(t_request *a, t_request *b);
void		sift_up(t_heap *h, int i);
void		sift_down(t_heap *h, int i);

/* dongle.c / acquire.c */
void		dongle_enqueue(t_dongle *d, t_request req);
int			dongle_is_free(t_dongle *d, long *available_at);
int			dongle_head(t_dongle *d);
void		dongle_take(t_dongle *d, t_coder *coder);
void		dongle_release(t_dongle *d, t_coder *coder);
int			dongle_acquire_pair(t_coder *coder);

/* event.c */
void		coder_notify(t_coder *coder);
void		coder_wait(t_coder *coder, long until_us);
void		notify_window(t_sim *sim, int dongle_idx);
void		notify_all_coders(t_sim *sim);

/* coder.c / monitor.c */
void		*coder_routine(void *arg);
void		*monitor_routine(void *arg);

/* init.c / threads.c / cleanup.c */
int			init_sim(t_sim *sim);
int			start_threads(t_sim *sim);
void		join_threads(t_sim *sim);
void		cleanup_sim(t_sim *sim);

#endif
