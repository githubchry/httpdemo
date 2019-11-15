#pragma once

#include <pthread.h>
#include <stdbool.h>
#ifdef __cplusplus

class PthreadMutexLocker
{
protected:
	pthread_mutex_t * m_mutex_ptr;

public:
	PthreadMutexLocker( pthread_mutex_t &mutex )
	{
		m_mutex_ptr = &mutex;
		pthread_mutex_lock( m_mutex_ptr );
	}

	PthreadMutexLocker( volatile pthread_mutex_t &mutex )
	{
		m_mutex_ptr = ( pthread_mutex_t * ) & mutex;
		pthread_mutex_lock( m_mutex_ptr );
	}

	~PthreadMutexLocker()
	{
		pthread_mutex_unlock( m_mutex_ptr );
	}
};

#	define PTHREAD_MUTEX_OBJECT_LOCKER( mutex, number ) PthreadMutexLocker __mutex_locker_##number( mutex );

#endif

#define PTHREAD_MUTEX_INIT( mutex, mutex_type ) { pthread_mutexattr_t mutex_attr; pthread_mutexattr_init( &mutex_attr ); pthread_mutexattr_settype( &mutex_attr, mutex_type ); pthread_mutex_init(( pthread_mutex_t* )&mutex, &mutex_attr ); pthread_mutexattr_destroy( &mutex_attr ); }
#define PTHREAD_MUTEX_DESTROY( mutex ) pthread_mutex_destroy( (pthread_mutex_t *) &mutex )

#if ( __GNUC__ >= 3 ) || defined(_MSC_VER)
#	define PTHREAD_MUTEX_LOCKER( mutex, ... ) ( pthread_mutex_lock( (pthread_mutex_t *)&mutex ), __VA_ARGS__ , pthread_mutex_unlock( (pthread_mutex_t *)&mutex ) )
#else
#	define PTHREAD_MUTEX_LOCKER( mutex, args... ) ( pthread_mutex_lock( (pthread_mutex_t *)&mutex ), args ,pthread_mutex_unlock( (pthread_mutex_t *)&mutex ) )
#endif


/*
 * these functions is to stimulate a new pthread_create and pthread_join for 
 * avoid the memory leak in some pthread library ( some version would ).
 */
 
/**
 * the possible states of the thread ("=>" shows all possible transitions from
 * this state)
 */
typedef enum _ry_pthread_state_t
{
	FVPS_NEW, ///< didn't start execution yet (=> RUNNING)
	FVPS_RUNNING, ///< thread is running (=> PAUSED, CANCELED)
//	FVPS_PAUSED, ///< thread is temporarily suspended (=> RUNNING)
//	FVPS_CANCELED, ///< thread should terminate a.s.a.p. (=> EXITED)
	FVPS_EXITED, ///< thread is terminating
} ry_pthread_state_t;

typedef struct _ry_pthread_t ry_pthread_t;
typedef void *(* ry_pthread_function_t ) ( ry_pthread_t * ) ;

struct _ry_pthread_t
{
	/// it will be invalid when state == FVPS_EXITED 
	pthread_t 				context; 
	ry_pthread_state_t 		state;
	_Bool 					is_destroy;
	ry_pthread_function_t 	function;
	void *					data;
	void *					result;
};


#ifdef __cplusplus
extern "C" {
#endif

/**
 * the thread created here is only a create detached thread, you could control it
 * with the normally thread control functions.
 * 
 * we only have 2 action : create, destroy. and we thought it's enough!
 * 
 * remenber do not use PTHREAD_CREATE_DETACHED at the attribute.........
 */ 
ry_pthread_t * ry_pthread_create(
	const pthread_attr_t * attribute,
    ry_pthread_function_t thread_function,
    void * data );

/**
 * stop and destroy all resources releate to the thread, we would wait infinity
 * until thread exited. so, keep you eye clear ..
 * 
 * @param [in] self_ptr pointer to the pthread context
 * @return result returned from thread
 */
void * ry_pthread_destroy( ry_pthread_t * * self_ptr );

#ifdef __cplusplus
}
#endif


    
#ifndef ry_pthread_get_is_destroy
#	define ry_pthread_get_is_destroy( self ) ((const int)(self)->is_destroy)
#endif

#ifndef ry_pthread_get_state
#	define ry_pthread_get_state( self ) ((const ry_pthread_state_t)(self)->state)
#endif

#ifndef ry_pthread_get_result
#	define ry_pthread_get_result( self ) ((self)->result)
#endif

#ifndef ry_pthread_get_data
#	define ry_pthread_get_data( self ) ((self)->data)
#endif
