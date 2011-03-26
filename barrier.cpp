
#include "barrier.h"

#ifdef WITH_BARRIER

int
barrier_init(barrier_t *barrier, int count)
{
  barrier->total = count;
	barrier->current = 0;
  pthread_mutex_init(&barrier->mutex, NULL);
  pthread_cond_init(&barrier->cond, NULL);
  return 0;
}

int
barrier_destroy(barrier_t *barrier)
{
  pthread_mutex_destroy(&barrier->mutex);
  pthread_cond_destroy(&barrier->cond);
  return 0;
}

int
barrier_wait(barrier_t *barrier)
{
  pthread_mutex_lock(&barrier->mutex);

  barrier->current++;

  if (barrier->current == barrier->total) {
		barrier->current = 0;
    pthread_cond_broadcast(&barrier->cond);
	} else
    pthread_cond_wait(&barrier->cond, &barrier->mutex);

  pthread_mutex_unlock(&barrier->mutex);

  return 0;
}

#endif /* WITH_BARRIER */
