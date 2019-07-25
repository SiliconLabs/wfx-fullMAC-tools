#include  <common/source/kal/kal_priv.h>
#include <athdefs.h>

extern A_STATUS a_mutex_init    (KAL_LOCK_HANDLE *mutex);
extern A_STATUS a_mutex_lock    (KAL_LOCK_HANDLE *mutex);
extern A_STATUS a_mutex_unlock  (KAL_LOCK_HANDLE *mutex);
extern A_STATUS a_mutex_destroy (KAL_LOCK_HANDLE *mutex);
