
#include "rypthread.h"
#include "rymacros.h"
#include "ryprint.h"

#include <malloc.h>
#include <string.h>

#ifndef ry_pthread_set_is_destroy
#	define ry_pthread_set_is_destroy( self, value ) ( (self)->is_destroy = (value) )
#endif

static void * ry_pthread_funtion_internal( ry_pthread_t * self )
{
	/* keep safe if someone destroy self outside after self->state = FVPS_EXITED
	and before return .
	*/
	void * result = 0; 	
	
	pthread_detach( self->context );
	
	self->state = FVPS_RUNNING;
	
	result = self->result = self->function( self );		
	
	self->state = FVPS_EXITED;
	
	/* it's dangers during this time, hope it won't lead to more serious problem .
	*/
	return result;
}

ry_pthread_t * ry_pthread_create(
	const pthread_attr_t * attribute,
    ry_pthread_function_t thread_function,
    void * data )
{
	ry_pthread_t * 		result = 0;
	int					error_code = 0;
	/* WARNING : we use joinable and detached in the begin of thread not 
	because of we want to do that, just because it will crash when we using
	the PTHREAD_CREATE_DETACHED ! ( gcc 4.1.1 uclinux 06 ).
	*/
	int					detach_state = PTHREAD_CREATE_JOINABLE; 
	
	result = (ry_pthread_t *) malloc( sizeof(ry_pthread_t) );
	if( !result )
	{
		ryErr( "failed to allocate pthread memory!\n" );
		goto LABEL_PTHREAD_CREATE_FAILED;
	}
	
	memset( result, 0, sizeof(ry_pthread_t) );	
	
	if( attribute )
	{		
		/* TODO: actually, we should check the function value it returned   */
		if( pthread_attr_getdetachstate( attribute, &detach_state ) )
		{
			ryErr( "can\'t get the detach state!\n" );
			goto LABEL_PTHREAD_CREATE_FAILED;
		}
		
		if( PTHREAD_CREATE_JOINABLE != detach_state )
		{
			ryErr( "the thread must be create joinable!\n" );
			goto LABEL_PTHREAD_CREATE_FAILED;
		}
	}
	
	result->state = FVPS_NEW;
	result->is_destroy = FALSE;
	result->data = data;
	result->function = thread_function;
	
	error_code = pthread_create(
		&result->context,
		attribute,
		(void *(*) ( void * ))ry_pthread_funtion_internal,
		result );	
	if( 0 != error_code )
	{
		ryErr( "failed to create thread!\n" );
		goto LABEL_PTHREAD_CREATE_FAILED;
	}	
	
	return result;
	
LABEL_PTHREAD_CREATE_FAILED:	
	if( result )
	{
		free( result );
	}

	return 0;
}
    
void * ry_pthread_destroy( ry_pthread_t * * self_ptr )
{
	void * result = 0;
	
	if( !self_ptr )
	{
		return 0 ;
	}
	
	if( !(*self_ptr) )
	{
		return 0 ;
	}
	
	ry_pthread_set_is_destroy( *self_ptr, TRUE );
	
	while( FVPS_EXITED != ry_pthread_get_state( *self_ptr ) )
	{
		usleep(1000); /* sleep 1 ms */
	}
	
	result = ry_pthread_get_result( *self_ptr );
	
	free( *self_ptr );
	*self_ptr = 0;
	
	return result;
}

