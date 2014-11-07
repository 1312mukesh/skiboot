#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>
#include <ccan/list/list.h>

struct timer;

typedef void (*timer_func_t)(struct timer *t, void *data);

/* Structure exposed in order to be able to allocate it
 * statically but otherwise, use accessors, don't access
 * the fields directly
 *
 * WARNING: Do not free a timer object unless you have cancelled
 * it first or you know it won't reschedule itself and have done
 * a sync_timer() on it. The timer core *will* access the object
 * again after you return from the expiry callback so it must not
 * be freed from the callback itself.
 */
struct timer {
	struct list_node	link;
	uint64_t		target;
	timer_func_t		expiry;
	void *			user_data;
	void *			running;
};

extern void init_timer(struct timer *t, timer_func_t expiry, void *data);

/* (re)schedule a timer. If already scheduled, it's expiry will be updated
 *
 * This doesn't synchronize so if the timer also reschedules itself there
 * is no telling which one "wins". The advantage is that this can be called
 * with any lock held or from the timer expiry itself.
 */
extern void schedule_timer(struct timer *t, uint64_t how_long);
extern void schedule_timer_at(struct timer *t, uint64_t when);

/* Synchronization point with the timer. If the callback has started before
 * that function is called, it will be complete when this function returns.
 *
 * It might start *again* but at least anything before this will be visible
 * to any subsequent occurrence.
 *
 * The usual issue of such sync functions exist: don't call it while holding
 * a lock that the timer callback might take or from the timer expiry itself.
 */
extern void sync_timer(struct timer *t);

/* cancel_timer() will ensure the timer isn't concurrently running so
 * the cancellation is guaranteed even if the timer reschedules itself.
 *
 * This uses sync_timer() internally so don't call this while holding a
 * lock the timer might use.
 */
extern void cancel_timer(struct timer *t);

/* cancel_timer_async() allows to remove the timer from the schedule
 * list without trying to synchronize. This is useful if the cancellation
 * must happen while holding locks that would make the synchronization
 * impossible. The user is responsible of ensuring it deals with potentially
 * spurrious occurrences
 */
extern void cancel_timer_async(struct timer *t);

/* Run the timers */
extern void check_timers(void);

#endif /* __TIMER_H */