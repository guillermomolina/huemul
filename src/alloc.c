/***************************************************************************
 *   Huemul Smalltalk                                                      *
 *   Copyright (C) 2007 by Guillermo Adrián Molina                         *
 *   huemul@losmolina.com.ar                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the Huemul License as described in the LICENSE  *
 *   file included in this project                                         *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *                                                                         *
 *   You should have received a copy of the Huemul License along with this *
 *   program; if not, write to huemul@losmolina.com.ar                     *
 *                                                                         *
 *   For a newer version of this file see:                                 *
 *    - http://www.guillermomolina.com.ar/huemul/                          *
 ***************************************************************************/


#include "huemul.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct _STFreeObject {
	CObjectPointer class;
	STInteger *size;
	struct _STFreeObject *next;
} STFreeObject;

#define InitialObjectMemorySize 256*1024*1024 /* Must be page aligned */
#define DefaultAditionalChunkSize 128*1024*1024 /* Must be page aligned */
STFreeObject* firstFreeObject;
STObject* startOfObjectMemory;
size_t objectMemorySize = 0;
size_t maxObjectMemorySize = MaxMemorySize;
size_t usedMemorySize = 0;
pthread_mutex_t garbageCollectorMutex = PTHREAD_MUTEX_INITIALIZER;
size_t lowSpaceWatcherThreshold = MaxMemorySize*0.1;
CObjectPointer waitingForGC = generalError;
CObjectPointer lowSpaceSemaphore = generalError;

/*
void setSystemMaxMem( void ) {
	struct rlimit limit;
	limit.rlim_cur = (rlim_t)MaxMemorySize;
	limit.rlim_max = (rlim_t)MaxMemorySize;
	setrlimit(RLIMIT_AS, &limit);
}
*/

CObjectPointer inCriticalSection = generalError;

inline
int enterCriticalSection( void ) {
	sigset_t signal_mask;
	sigemptyset (&signal_mask);
	sigaddset( &signal_mask, SIGUSR1 );
	int rc = pthread_sigmask (SIG_SETMASK, &signal_mask, NULL);
#ifdef DEBUG
	if (rc != 0)
		error("enterCriticalSection: Can not set signal mask");
	inCriticalSection = true;
#endif
	return rc;
}

inline
int leaveCriticalSection( void ) {
	sigset_t signal_mask;
	sigemptyset (&signal_mask);
	int rc = pthread_sigmask (SIG_SETMASK, &signal_mask, NULL);
#ifdef DEBUG
	if (rc != 0)
		error("leaveCriticalSection: Can not set signal mask");
	inCriticalSection = false;
#endif
	return rc;
}

extern CObjectPointer LargePositiveInteger;
CObjectPointer newIntegerOrLong( unsigned int value ) {
	if( ( value & 0xC0000000 ) == 0 )
		return (CObjectPointer)newInteger(value);
	CObjectPointer num = newVariableByteObject( LargePositiveInteger, newInteger(4) );
	STArray *number = (STArray *)detagObject( num );
	number->array[0] = (CObjectPointer)value;
	return num;
}
			
inline
STObject *setClass(STObject *object, CObjectPointer class) {
	object->header.class = class;
}


void setFreeObject( STFreeObject *object, STInteger *size, STFreeObject *next ) {
	setClass( ( STObject *)object, generalError );
	object->size = size;
	object->next = next;
}

inline
unsigned int roundToLong(unsigned int i) {
	unsigned int a=(i+3)&0xFFFFFFFC;
	return a;
}

void freeObject( CObjectPointer ptr ) {
	STObject *object = detagObject( ptr );
#ifdef DEBUG
	if( !object )
		error("freeObject: ptr == NULL");
#endif
	STFreeObject *previousFreeObject = firstFreeObject;
	firstFreeObject = (STFreeObject *)object;
	size_t size = roundToLong(getValue(firstFreeObject->size));
	setFreeObject( firstFreeObject, newInteger(size),previousFreeObject );
	usedMemorySize -= size + sizeof(STHeader);
}

inline
STObject *setClassAndSize( STObject *object, CObjectPointer class, STInteger *size ) {
	setClass( object, class );
	object->header.size = size;
	STClass *oClass = (STClass *)detagObject(class);
	unsigned int info = newHash() | (((unsigned int)oClass->format.type) << 12) | (((unsigned int)oClass->format.weak) << 14);
	int *obj = (int *)object;
	obj[2] = (int) newInteger(info);
	return object;
}

CObjectPointer checkObjectHeader( CObjectPointer obj );
CObjectPointer createObjectOneTry( CObjectPointer class, STInteger *size ) {
	/* size is bytes */
	size_t roundedSize = roundToLong( getValue( size ) );
#ifdef DEBUG
//	if( roundedSize > MaxObjectSize )
//		error("createObject: Trying to create a big object");
#endif
	size_t roundedSizeWithHeader = roundedSize + sizeof(STObject);
	enterCriticalSection();
	pthread_mutex_lock( &garbageCollectorMutex );
	STFreeObject *freeObject = firstFreeObject;
	STFreeObject *previousFreeObject = NULL;
	while( freeObject ) {
		size_t freeObjectSize = getValue( freeObject->size );
		STFreeObject *nextFreeObject = freeObject->next;
		if( roundedSizeWithHeader <= freeObjectSize ) {
			if( roundedSize == freeObjectSize ) {
				if( previousFreeObject )
					previousFreeObject->next = nextFreeObject;
				else
					firstFreeObject = nextFreeObject;
			}
			else {
				STFreeObject *nextFreeObject = (STFreeObject *)( (size_t)freeObject + roundedSizeWithHeader );
#ifdef DEBUG
				if( (size_t)nextFreeObject > (size_t)startOfObjectMemory + objectMemorySize )
					error("createObject: Can not be");
#endif
				if( previousFreeObject )
					previousFreeObject->next = nextFreeObject;
				else
					firstFreeObject = nextFreeObject;
				setFreeObject( nextFreeObject, newInteger( freeObjectSize - roundedSizeWithHeader ), freeObject->next );
			}			
			STObject *newObject = (STObject *)freeObject;
			setClassAndSize( newObject, class, size );
			usedMemorySize += roundedSizeWithHeader;
			pthread_mutex_unlock( &garbageCollectorMutex );
#ifdef DEBUG
			if( checkObjectHeader( tagObject( newObject ) ) == false ) {
				printf("WARNING: createObject: Wrong header in:\n");
				print( newObject );
				return false;
			}
#endif
			int i;
			switch( newObject->header.info.type ) {
				case FixedObject: //For the inlined size
				case VariableObject:
					for( i=0;i<roundedSize>>2;i++)
						((STArray *)newObject)->array[i] = nil;
					break;
				case VariableByteObject:
				case VariableWordObject:
					memset( ((STArray *)newObject)->array, 0, roundedSize );
			};
			leaveCriticalSection();
			return tagObject( newObject );
		}
		previousFreeObject = freeObject;
		freeObject = freeObject->next;
	}
	pthread_mutex_unlock( &garbageCollectorMutex );
	leaveCriticalSection();
	return generalError;
}

void triggerLowSpaceWatcher( void ) {
	while( waitingForGC == true )
		sched_yield();
	waitingForGC = true;
	if( lowSpaceSemaphore == generalError )
		error("createObject: Low space semaphore not set, and memory is low");
	primitiveSemaphoreSignal( lowSpaceSemaphore );
	while( waitingForGC == true )
		sched_yield();
}

CObjectPointer createObject( CObjectPointer class, STInteger *size ) {
	size_t oldLowSpaceWatcherThreshold = lowSpaceWatcherThreshold;
	CObjectPointer newObject;
	size_t objectSize = roundToLong( getValue(size) ) + sizeof(STHeader);
	if( waitingForGC != true && ((usedMemorySize + lowSpaceWatcherThreshold ) >= objectMemorySize ) )
		triggerLowSpaceWatcher();
	while( ( newObject = createObjectOneTry( class, size ) ) == generalError ) {
		lowSpaceWatcherThreshold += oldLowSpaceWatcherThreshold;
		triggerLowSpaceWatcher();
	}
	lowSpaceWatcherThreshold = oldLowSpaceWatcherThreshold;
	return newObject;
}

CObjectPointer loadFileIntoMemory( char* imageName ) {
	int imageFile;
	struct stat statbuf;
	
#ifdef DEBUG
	printf("Opening image file: %s...", imageName );
#endif
	imageFile = open (imageName, O_RDONLY);
	if( imageFile < 0) {
		printf("Can't open image file: %s\n", imageName );
		return generalError;
	}
#ifdef DEBUG
	printf(" done\nStatting image file..." );
#endif
	if (fstat (imageFile,&statbuf) < 0) {
		printf("Can't stat image file\n" );
		return generalError;
	}
	usedMemorySize = statbuf.st_size;
	if( (usedMemorySize + sizeof(STHeader)) > objectMemorySize ) {
		printf( "Image is too big to fit in memory\n" );
		return generalError;
	}
#ifdef DEBUG
	printf(" done\nCopying image file..." );
#endif
	if( read( imageFile, startOfObjectMemory, usedMemorySize ) != usedMemorySize ) {
		printf("Can't copy image file\n" );
		return generalError;
	}
#ifdef DEBUG
	printf(" done\nClosing image file..." );
#endif
	if (close(imageFile) < 0 ) {
		printf("Can't close image file\n" );
		return generalError;
	}
#ifdef DEBUG
	printf(" done\n" );
#endif
	firstFreeObject = (STFreeObject *)((size_t)startOfObjectMemory + usedMemorySize);
	setFreeObject( firstFreeObject, newInteger( objectMemorySize - usedMemorySize - sizeof(STObject) ), NULL );
	return tagObject(startOfObjectMemory);
}

void *lowestAddress( int file ) {
	size_t pageSize = getpagesize();
#define ReserveHeapSize 128*1024*1024
	char *memoryAddress = (char *)sbrk(0) + ReserveHeapSize;
	while( mmap( memoryAddress, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC , MAP_FIXED | MAP_PRIVATE, file, 0) != memoryAddress ) {
		perror( NULL );
		memoryAddress += pageSize;
	};
	munmap( memoryAddress, pageSize );
	return memoryAddress;
}

CObjectPointer initializeObjectMemory( void ) {
	int file;
#ifdef DEBUG
	printf( "Opening /dev/zero..." );
#endif
	file = open ( "/dev/zero", O_RDONLY);
	if( file < 0) {
		printf("Can't open /dev/zero\n" );
		exit(-1);
	}
#ifdef DEBUG
	printf(" done\nMapping /dev/zero..." );
#endif
	objectMemorySize = InitialObjectMemorySize;
#ifdef DEBUG
	if( objectMemorySize % getpagesize() != 0 )
		error( "InitialFreeObjectMemory must be multiple of page size" );
#endif	
	if( objectMemorySize > maxObjectMemorySize ) {
		printf( "Initial memory is to big\n" );
		exit(-1);
	}
	startOfObjectMemory = (STObject *)mmap( lowestAddress( file ), objectMemorySize, PROT_READ | PROT_WRITE | PROT_EXEC , MAP_FIXED | MAP_PRIVATE, file, 0);
	if ( startOfObjectMemory == (STObject *)-1) {
		printf("Can't map /dev/zero\n" );
		exit(-1);
	}
#ifdef DEBUG
	printf(" done\nClosing /dev/zero..." );
#endif
	if (close(file) < 0 ) {
		printf("Can't close /dev/zero\n" );
		exit(-1);
	}
#ifdef DEBUG
	printf(" done\n" );
#endif
	firstFreeObject = (STFreeObject *)startOfObjectMemory;
	setFreeObject( firstFreeObject, newInteger( objectMemorySize - sizeof(STObject) ), NULL );
	return tagObject(startOfObjectMemory);
}

CObjectPointer checkAllMemory( void );
void getMoreCore( void ) {
#ifdef DEBUG
	if( DefaultAditionalChunkSize % getpagesize() != 0 )
		error("DefaultAditionalChunkSize is not page aligned");
#endif
	size_t newObjectMemorySize = objectMemorySize + DefaultAditionalChunkSize;
	if( newObjectMemorySize > maxObjectMemorySize )
		error( "Too much memory, increase maxObjectMemorySize" );
	enterCriticalSection();
	pthread_mutex_lock( &garbageCollectorMutex );	
	void *retValue = mremap( startOfObjectMemory, objectMemorySize, newObjectMemorySize, 0 );
	if ( retValue == (void *)-1)
		error("Can not get more core memory");
	if ( retValue != (void *)startOfObjectMemory )
		error("Object Memory moved address");
	STFreeObject *newFreeObject = (STFreeObject *)((size_t)startOfObjectMemory + objectMemorySize);
	objectMemorySize = newObjectMemorySize;
	STFreeObject *previousFreeObject = firstFreeObject;
	firstFreeObject = (STFreeObject *)newFreeObject;
	setFreeObject( firstFreeObject, newInteger( DefaultAditionalChunkSize - sizeof(STObject) ), previousFreeObject );
#ifdef DEBUG
	initializeAllocatedObjects();
	if(checkAllMemory()==false)
		error("Something wrong in memory before GC\n");
#endif
	pthread_mutex_unlock( &garbageCollectorMutex );
	leaveCriticalSection();
}

CObjectPointer nextObject( CObjectPointer previous ) {
	size_t firstUnallocatedPointer = (size_t)startOfObjectMemory + objectMemorySize;
	STObject *object = detagObject(previous);
	while( object ) {
		size_t objectSize = roundToLong( getValue( object->header.size) ) + sizeof( STHeader );
		object = (STObject *)((size_t)object + objectSize);
		if( (size_t)object == firstUnallocatedPointer )
			return generalError;
#ifdef DEBUG
		if( (size_t)object > firstUnallocatedPointer )
			error("Can not be");
#endif
		if( object->header.class != generalError )
			return tagObject(object);
	} 
}

CObjectPointer bytesLeft( void ) {
	return newIntegerOrLong( objectMemorySize - usedMemorySize );
}

int getSize(CObjectPointer rcvr) {
	STArray *array = (STArray *)detagObject(rcvr);
	switch( array->header.info.type ) {
		case FixedObject: //For the inlined size
			error("Fixed object in getSize()");
			break;
		case VariableObject:
			return getValue(array->header.size)>>2;
		case VariableByteObject:
			return getValue(array->header.size);
		case VariableWordObject:
			return getValue(array->header.size)>>2;
	};
	error("Bad colection in #size");
}

pthread_mutex_t hashMutex = PTHREAD_MUTEX_INITIALIZER;
int newHash( void ) {
	static int lastHash = 1163;
	pthread_mutex_lock( &hashMutex );
	lastHash = 13849 + ( ( 27181 * lastHash ) & 65535 );
	pthread_mutex_unlock( &hashMutex );
	return ( lastHash & 0xFFF );
}

#ifdef DEBUG
typedef struct {
	CObjectPointer class;
	int count;
	int size;
} ClassInstance;

#define ClassCountSize	256
int nextClassCount = 0;
ClassInstance classCount[ ClassCountSize ];

extern CObjectPointer Symbol;
void countNewInstanceOfClass( CObjectPointer class, STInteger *size ) {
	int i;
	static CObjectPointer a = generalError;
//	if( getClass(((STClass *)detagObject(class))->name) != Symbol )
//		error("New class");
	for( i = 0; i < nextClassCount; i++ ) {
		if( classCount[i].class == class ) {
			classCount[i].count++;
			classCount[i].size += getValue(size) + sizeof( STHeader );
			return;
		}
	}
	if( nextClassCount >= ClassCountSize )
		error("Increase ClassCountSize");
	classCount[nextClassCount].class = class;
	classCount[nextClassCount].count = 1;
	classCount[nextClassCount].size = getValue(size) + sizeof( STHeader );
	nextClassCount++;
}

void printClassInstanceCount( void ) {
	int i;
//	printf("printClassInstanceCount: Biggest consummers:\n");
	for( i = 0; i < nextClassCount; i++ )
		if( classCount[i].size >  MaxMemorySize / 1000 ) {
			printf("%d instances of ", classCount[i].count );
			printString(getClassName(classCount[i].class));
			printf(", total bytes: %d\n", classCount[i].size );
		}
}
#endif

CObjectPointer objectClone( CObjectPointer src ) {
	if( isSmallInteger( src ) )
		error("Can not clone Smallinteger");
	STArray *source = (STArray *)detagObject( src );
	CObjectPointer newObject = createObject( source->header.class, source->header.size );
	STArray *object = (STArray *)detagObject( newObject );
	memcpy( object->array, source->array, getValue(source->header.size) );
	return newObject;
}

//
CObjectPointer newVariableByteObject(CObjectPointer class, STInteger *size) {
	CObjectPointer newObject = createObject( class, size );
	STArray *object = (STArray *)detagObject( newObject );
	return newObject;
}

// new variable object of Words 
CObjectPointer newVariableWordObject(CObjectPointer class, STInteger *size) {
	// size is dwords (4 bytes) 
	int sizeInWords = getValue(size);
	int sizeInBytes = sizeInWords << 2;
	CObjectPointer newObject = createObject( class, newInteger(sizeInBytes) );
	STArray *object = (STArray *)detagObject( newObject );
	return newObject;

}

// new variable object 
CObjectPointer newObject(CObjectPointer class, STInteger *size) {
	// size is dwords (4 bytes) 
	int sizeInWords = getValue(size);
	int sizeInBytes = sizeInWords << 2;
	CObjectPointer newObject = createObject( class, newInteger(sizeInBytes) );
	return newObject;
}

CObjectPointer instantiateClass(CObjectPointer class) {
	STClass *oClass = (STClass *)detagObject(class);
	return newObject(class, oClass->instanceSize);
}

extern CObjectPointer String;
CObjectPointer newString(char buffer[]) {
	int lenght=strlen(buffer);
	CObjectPointer str = newVariableByteObject(String,newInteger(lenght));
	STString *string=(STString *)detagObject(str);
	strncpy((char *)string->string,buffer,lenght);
	return str;
}

extern CObjectPointer Float;
CObjectPointer newFloat(double value) {
	CObjectPointer flt = newVariableByteObject(Float,newInteger(8));
	STFloat *floatValue = (STFloat *)detagObject(flt);
	floatValue->value = value;
	return flt;
}

extern CObjectPointer ExternalAddress;
CObjectPointer newExternalAddress( void *pointer ) {
	CObjectPointer addr = newVariableByteObject( ExternalAddress, newInteger( 4 ) );
	STArray *address = ( STArray * )detagObject( addr );
	address->array[ 0 ] = (int)pointer; 
	return addr;
}

CObjectPointer getFirstInstanceOf( CObjectPointer class ) {
	if( class == SmallInteger )
		return nil;
	CObjectPointer object = tagObject( startOfObjectMemory );
	while( object != generalError ) {
		if( getClass(object) == class ) 
			return object;
		object = nextObject( object );
	}
	return nil;
}

CObjectPointer getNextInstanceAfter( CObjectPointer previousObject ) {
	//First and slow implementation of this function
	int i;
	CObjectPointer class;
	if( (class = getClass(previousObject)) == SmallInteger )
		return nil;
	CObjectPointer object = nextObject( previousObject );
	while( object != generalError ) {
		if( getClass(object) == class ) 
			return object;
		object = nextObject( object );
	}
	return nil;
}

#define WeakObjectListSize	102400
unsigned int nextWeakObject = 0;
CObjectPointer weakObjectList[ WeakObjectListSize ];

extern CObjectPointer Smalltalk;
void markObjectRecursivelly( CObjectPointer obj ) {
	STArray *object = (STArray *)detagObject( obj );
	if(!object->header.info.marked) {
		object->header.info.marked = 1;
		markObjectRecursivelly( object->header.class );
		if(object->header.info.weak) {
			if(nextWeakObject >= WeakObjectListSize)
				error("Weak Array too small");
			weakObjectList[nextWeakObject++] = obj;
			return;
		}
		int i;
		if( object->header.info.type == FixedObject || object->header.info.type == VariableObject ) 
			for( i = 0; i < getValue(object->header.size) >> 2; i++ )
				if(!isSmallInteger(object->array[i]))
					markObjectRecursivelly( object->array[i] );
	}
}

void clearObjecstMark( void ) {
	nextWeakObject = 0;
	CObjectPointer object = tagObject( startOfObjectMemory );
	while( object != generalError ) {
		detagObject( object )->header.info.marked = 0;
		object = nextObject( object );
	}
}

STFreeObject *nextFreeObject( STFreeObject *previous ) {
	STFreeObject *freeObject = previous;
	int i=0;
	while( freeObject ) {
		freeObject = (STFreeObject *)((size_t)freeObject + roundToLong( getValue( freeObject->size ) ) + sizeof( STFreeObject ));
		if( (size_t)freeObject >= ((size_t)startOfObjectMemory + objectMemorySize) )
			return NULL;
		if( freeObject->class == generalError )
			return freeObject;
		i++;
	}
}

void compactObjectMemory( void ) {
	firstFreeObject = nextFreeObject( (STFreeObject *)startOfObjectMemory );
	STFreeObject *freeObject = firstFreeObject;
	while( freeObject ) {
		STFreeObject *nextfreeObject = (STFreeObject *)((size_t)freeObject + getValue( freeObject->size ) + sizeof( STFreeObject ));
		if( nextfreeObject == nextFreeObject(freeObject) ) {
			freeObject->size = newInteger( getValue(freeObject->size) + sizeof(STHeader) + getValue(nextfreeObject->size) );
		}
		else {
			freeObject->next = nextFreeObject( freeObject );
			freeObject = freeObject->next;
		}
	}
#ifdef DEBUG
	freeObject = firstFreeObject;
	while( freeObject ) {
		STFreeObject *nextfreeObject = (STFreeObject *)((size_t)freeObject + getValue( freeObject->size ) + sizeof( STFreeObject ));
		if( nextfreeObject == nextFreeObject(freeObject) )
			error("compactObjectMemory: Memory is not compacted ok");
		if( freeObject->next != nextFreeObject(freeObject) )
			error("compactObjectMemory: Memory is not compacted ok");
		freeObject = nextFreeObject( freeObject );
	}
#endif
}

extern void *mainStackTop;
extern CObjectPointer symbol_running;
extern CObjectPointer symbol_terminated;
extern CObjectPointer symbol_initialized;
CObjectPointer isKnownObject( CObjectPointer object );
void markObjectsInProcess( CObjectPointer proc ) {
	STProcess *process = (STProcess *)detagObject( proc );
	if( process->state == symbol_initialized || process->state == symbol_terminated )
		return;
	STArray *stackPointer = (STArray *)detagObject( process->stackPointer );
	unsigned int *basePointer;
	if( process->state == symbol_running ) //The only running process
		basePointer = (unsigned int *)&proc; // tricky, used to get base pointer
	else
		basePointer = (unsigned int *)stackPointer->array[ 0 ];
	STArray *stackTop = (STArray *)detagObject( process->stackTop );
	if( process->stackTop == nil || stackTop->array[ 0 ] == (CObjectPointer)NULL ) { //niled at startup
		printf("WARNING: Main stack top is nil\n");
		stackTop->array[ 0 ] = (CObjectPointer)mainStackTop;
	}
	int stackSize = ( stackTop->array[ 0 ] - (unsigned int)basePointer ) >> 2;
#ifdef DEBUG
	if(stackSize>4096) {
		print(proc);
		error("Bad stack size reported");
	}
#endif
	int i;
	for( i = 0; i < stackSize ; i++ )
		if( isKnownObject(basePointer[i]) == true )
			markObjectRecursivelly( basePointer[i] );			
}

void freeUnusedObjects( void ) {
	CObjectPointer object = tagObject( startOfObjectMemory );
	while( object != generalError ) {
		if( detagObject( object )->header.info.marked == 0 )
			freeObject( object );
		object = nextObject( object );
	}
}

CObjectPointer finalizeObjects( void ) {
	CObjectPointer needFinalization  = false;
	if(nextWeakObject) {
		int j;
		for( j = 0; j < nextWeakObject; j++ ) {
			int i;
			STArray *array = (STArray *)detagObject( weakObjectList[j] );
			for( i = 0; i < getValue(array->header.size) >> 2; i++ ) {
				STObject *object = detagObject(array->array[i]);
				if(!isSmallInteger(array->array[i]) && !object->header.info.marked) {
					array->array[i] = nil;
					needFinalization  = true;
				}
			}
		}
	}
	return needFinalization;
}

typedef struct{
	STHeader header;
	CObjectPointer processList;
}STProcessorScheduler;

typedef struct{
	STHeader header;
	CObjectPointer tally;
	CObjectPointer array;
	CObjectPointer flag;
}STWeakArray;

void initializeMethodCache( void );
extern CObjectPointer Processor;
extern CObjectPointer Process;
CObjectPointer garbageCollectMost( void ) {
#ifdef DEBUG
	printf("garbageCollect: Start of GC\n");
#endif
	initializeAllocatedObjects();
#ifdef DEBUG
//	printClassInstanceCount();
	if(checkAllMemory()==false)
		printf("WARNING: garbageCollect: Something wrong in memory before GC\n");
#endif
	clearObjecstMark();
	markObjectRecursivelly( Smalltalk );
	STProcessorScheduler *scheduler = (STProcessorScheduler *)detagObject( Processor );
	STWeakArray *processList = (STWeakArray *)detagObject( scheduler->processList );
	STArray *processArray = (STArray *)detagObject( processList->array );
	int i;
#ifdef DEBUG
	int running = 0;
	for( i = 0; i < getSize(processList->array); i++ )
		if( getClass(processArray->array[i]) == Process ) {
			STProcess *process = (STProcess *)detagObject( processArray->array[i] );
			if( process->state == symbol_running ) {
				printf("GC within: ");
				printString(process->name);
				printf(".\n");
				running++;
			}
		}
	if( running != 1 )
		error("garbageCollect: Can not run GC number of running processes is not 1");
#endif
	for( i = 0; i < getSize(processList->array); i++ )
		if( getClass(processArray->array[i]) == Process )
			markObjectsInProcess( processArray->array[i] );
	CObjectPointer needFinalization = finalizeObjects();
	freeUnusedObjects();
	initializeMethodCache(); //Flush cache
	compactObjectMemory();
#ifdef DEBUG
	initializeAllocatedObjects();
	if(checkAllMemory()==false)
		printf("WARNING: garbageCollect: Something wrong in memory after GC\n");
	printf("garbageCollect: End of GC\n");
#endif
	waitingForGC = false;
	return needFinalization;
}

CObjectPointer garbageCollect( void ) {
	return garbageCollectMost();
}

CObjectPointer becomeInProcess( CObjectPointer rcvr, CObjectPointer arg, CObjectPointer twoWays, CObjectPointer proc ) {
	STProcess *process = (STProcess *)detagObject( proc );
	if( process->state == symbol_initialized || process->state == symbol_terminated )
		return;
	STArray *stackPointer = (STArray *)detagObject( process->stackPointer );
	unsigned int *basePointer;
	if( process->state == symbol_running ) //The only running process
		basePointer = (unsigned int *)&proc; // tricky, used to get base pointer
	else
		basePointer = (unsigned int *)stackPointer->array[ 0 ];
#ifdef DEBUG
	if( process->stackTop == nil )
		error("Should be initialized");
#endif
	STArray *stackTop = (STArray *)detagObject( process->stackTop );
	int stackSize = ( stackTop->array[ 0 ] - (unsigned int)basePointer ) >> 2;
#ifdef DEBUG
	if(stackSize>4096) {
		print(proc);
		error("Bad stack size reported");
	}
#endif
	int i;
	for( i = 0; i < stackSize ; i++ ) {
		if( twoWays == true && arg == basePointer[i] )
			basePointer[i] = rcvr;
		else if( rcvr == basePointer[i] )
			basePointer[i] = arg;
	}
}

CObjectPointer become( CObjectPointer rcvr, CObjectPointer arg, CObjectPointer twoWays );
CObjectPointer arrayBecome( CObjectPointer rcvr, CObjectPointer arg, CObjectPointer twoWays ) {
	/* 	All references to each object in array1 are swapped with all references to the corresponding
	object in array2. That is, all pointers to one object are replaced with with pointers to the other.
	The arguments must be arrays of the same length. */
	if( !isArray(rcvr) )
		return generalError;
	if( !isArray(arg) )
		return generalError;
	int size = getSize(rcvr);
	if( size != getSize(arg) )
		return generalError;
	STArray *receiver = (STArray *)detagObject(rcvr);
	STArray *argument = (STArray *)detagObject(arg);
	if( receiver->header.info.type != VariableObject || argument->header.info.type != VariableObject )
		return generalError;
	int i;

	STProcessorScheduler *scheduler = (STProcessorScheduler *)detagObject( Processor );
	STWeakArray *processList = (STWeakArray *)detagObject( scheduler->processList );
	STArray *processArray = (STArray *)detagObject( processList->array );
#ifdef DEBUG
	int running = 0;
	for( i = 0; i < getSize(processList->array); i++ )
		if( getClass(processArray->array[i]) == Process ) {
		STProcess *process = (STProcess *)detagObject( processArray->array[i] );
			if( process->state == symbol_running ) {
				printf("arrayBecome within: ");
				printString(process->name);
				printf(".\n");
				running++;
			}
		}
	if( running != 1 )
		error("arrayBecome: Can not run, number of running processes is not 1");
#endif
	initializeAllocatedObjects();
	for( i = 0; i < getSize(rcvr); i++ ) {
		int j;
		become( receiver->array[i], argument->array[i], twoWays );
		for( j = 0; j < getSize(processList->array); j++ )
			if( getClass(processArray->array[j]) == Process ) 
				becomeInProcess( receiver->array[i], argument->array[i], twoWays, processArray->array[j] );
	}
	
	initializeMethodCache();
	return rcvr;
}



