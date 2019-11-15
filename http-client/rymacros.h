#pragma once

#ifdef __cplusplus
#	define RY_DEFAULT_VALUE( type, var, value ) type var = value
#else
#	define RY_DEFAULT_VALUE( type, var, value ) type var
#endif

#ifndef MIN
#	define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif // #ifndef MIN

#ifndef MAX
#	define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif // #ifndef MAX

#ifndef SIZE_OF_ARRAY
#	define SIZE_OF_ARRAY( array ) (sizeof(array)/sizeof(array[0]))
#endif

#ifndef PTR_OFFSET_BYTES
#	define PTR_OFFSET_BYTES( ptr, offset ) ( (char *)(ptr) + (offset) )
#endif

#ifndef PTR_OFFSET_BYTES_FORWARD
#	define PTR_OFFSET_BYTES_FORWARD PTR_OFFSET_BYTES
#endif

#ifndef PTR_OFFSET_BYTES_BACKWARD
#	define PTR_OFFSET_BYTES_BACKWARD( ptr, offset ) PTR_OFFSET_BYTES( ptr, - (offset) )
#endif

#ifndef PTR_OFFSET_BETWEEN
#	define PTR_OFFSET_BETWEEN( from, to ) (((char*)to) - ((char*)from))
#endif

#ifndef PTR_OFFSET_UNITS
#	define PTR_OFFSET_UNITS( ptr, offset ) ( (ptr) + (offset) )
#endif

#ifndef PTR_OFFSET_UNITS_FORWARD
#	define PTR_OFFSET_UNITS_FORWARD PTR_OFFSET_UNITS
#endif

#ifndef PTR_OFFSET_UNITS_BACKWARD
#	define PTR_OFFSET_UNITS_BACKWARD( ptr, offset ) PTR_OFFSET_UNITS( ptr, - (offset) )
#endif

#ifndef MACRO_EMPTY_PARAMETER
#	define MACRO_EMPTY_PARAMETER
#endif

#ifndef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#endif

#define IF_TRUE_DO( condition, jobs ) if( condition ) { jobs; }
#define IF_FALSE_DO( condition, jobs ) if( !(condition) ) { jobs; }
#define IF_TRUE_RETURN( condition, result ) IF_TRUE_DO( condition, return result )
#define IF_FALSE_RETURN( condition, result ) IF_FALSE_DO( condition, return result )
#define IF_TRUE_GOTO( condition, label ) IF_TRUE_DO( (condition), goto label )
#define IF_FALSE_GOTO( condition, label ) IF_FALSE_DO( !(condition), goto label )
#define IF_FAILED_GOTO IF_FALSE_GOTO
#define IF_SUCCESSED_GOTO IF_TRUE_GOTO
#define IF_AFTER_EXECUTED_CONDITION_TRUE_GOTO( label, code_section, condition ) { { code_section ;}; if( condition ) goto label; };

#if ( __GNUC__ >= 3 ) || defined(_MSC_VER)
#	define IF_TRUE_OUTPUT_AND_DO( condition, code_blocks, string, ... ) if( condition ) { ryErr( string, ##__VA_ARGS__ ); code_blocks; }
#	define IF_TRUE_OUTPUT_AND_GOTO( condition, label, string, ... ) IF_TRUE_OUTPUT_AND_DO( (condition), goto label, string, ##__VA_ARGS__ );

#	define IF_TRUE_OUTPUT_AND_RETURN( condition, result, string, ... ) if( condition ) { ryErr( string, ##__VA_ARGS__ ); return result; }
#	define IF_TRUE_OUTPUT_AND_RETURN_VOID( condition, string, ... ) if( condition ) { ryErr( string, ##__VA_ARGS__ ); return ; }
#else // #if ( __GNUC__ >= 3 ) || defined(_MSC_VER)
#	define IF_TRUE_OUTPUT_AND_DO( condition, code_blocks, string, args... ) if( condition ) { ryErr( string, ##args ); code_blocks; }
#	define IF_TRUE_OUTPUT_AND_GOTO( condition, label, string, args... ) IF_TRUE_OUTPUT_AND_DO( (condition), goto label, string, ##args );

#	define IF_TRUE_OUTPUT_AND_RETURN( condition, result, string, args... ) if( condition ) { ryErr( string, ##args ); return result; }
#	define IF_TRUE_OUTPUT_AND_RETURN_VOID( condition, string, args... ) if( condition ) { ryErr( string, ##args ); return ; }
#endif // #if ( __GNUC__ >= 3 ) || defined(_MSC_VER)

#ifndef RY_NAME_SPACE_BEGIN
#	define RY_NAME_SPACE_BEGIN() namespace chry {
#endif

#ifndef RY_NAME_SPACE_END
#	define RY_NAME_SPACE_END() }
#endif

#ifndef RY_SUB_NAME_SPACE_BEGIN
#	define RY_SUB_NAME_SPACE_BEGIN( x ) RY_NAME_SPACE_BEGIN() namespace x {
#endif

#ifndef RY_SUB_NAME_SPACE_END
#	define RY_SUB_NAME_SPACE_END() RY_NAME_SPACE_END() }
#endif

#ifndef STRUCT_MEMBER_SIZE
#	define STRUCT_MEMBER_SIZE( struct_type, member_name ) sizeof((((struct_type*)0)->member_name))
#endif

#ifndef STRUCT_MEMBER_OFFSET
#	define STRUCT_MEMBER_OFFSET( struct_type, member_name ) ((size_t)(&(((struct_type*)0)->member_name)))
#endif

#ifndef STRUCT_MEMBER_PASSED_OFFSET

#	ifdef  __cplusplus
#		define STRUCT_MEMBER_PASSED_OFFSET( struct_type, member_name )  offsetof(struct_type, member_name)
#	else
/**
* @param struct_type struct type
* @param member_name struct member name
* @return offset after member of struct
*/
#		define STRUCT_MEMBER_PASSED_OFFSET( struct_type, member_name ) ((size_t)(&(((struct_type*)0)->member_name)) + sizeof(&(((struct_type*)0)->member_name)) )
#	endif
#endif

#ifndef STRUCT_GET_SIZE_BEFORE_MEMBER
#	define STRUCT_GET_SIZE_BEFORE_MEMBER STRUCT_MEMBER_OFFSET
#endif

#ifndef BOOL_VALUE
#	define BOOL_VALUE( condition ) ( (condition) ? TRUE : FALSE )
#endif /* #ifndef BOOL_VALUE */

#ifndef RY_INHERIT_INTERNAL_STRUCT
/**
* to simply declare a struct we want to separate from other structs and inherit
* from a known struct
*/
#	define RY_INHERIT_INTERNAL_STRUCT( name, base_type ) typedef struct name { base_type base; } name;
#endif/* #ifndef RY_DECLARE_INTERNAL_STRUCT */

/**
* to simply declare a struct we want to separate from other structs
*/
#ifndef RY_DECLARE_INTERNAL_STRUCT
#	define RY_DECLARE_INTERNAL_STRUCT( name ) RY_INHERIT_INTERNAL_STRUCT( name, int )
#endif


