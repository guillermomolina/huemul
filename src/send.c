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
#include <stdio.h>

#ifdef DEBUG
unsigned  int sendCount = 0;
#endif

#include <stdio.h>
#include <pthread.h>

extern CObjectPointer ProtoObject_doesNotUnderstandC58;

STCompiledMethod *findMethodinMethodDictionary( STString *methodName, STDictionary *mDict) {
   STCompiledMethod *mthd;
   STArray *methods= (STArray *)detagObject(mDict->array);
   int i;
   for( i=0; i<getValue(methods->header.size)>>2; i++) {
		STAssociation *association = (STAssociation *)detagObject(methods->array[i]);
		if(methodName==(STString *)detagObject(association->key) )
			return((STCompiledMethod *)detagObject(association->value));
   }
   return((STCompiledMethod *)detagObject(nil));
}

extern CObjectPointer Trait;
extern CObjectPointer ClassTrait;
STCompiledMethod *findMethodinTraitComposition( STString *methodName, STTraitComposition *traitComposition) {
	STCompiledMethod *method;
	int i;
	STArray *transformations = (STArray *)detagObject(traitComposition->transformations);
	for(i = 0; i < getSize(traitComposition->transformations); i++) {
		STClass *itemClass = (STClass *)detagObject(getClass(transformations->array[i]));
		STObject *item = detagObject(transformations->array[i]);
		if( item == detagObject(nil) )
			continue;
		if( itemClass == (STClass *)detagObject(Trait) || itemClass == (STClass *)detagObject(ClassTrait) ) {
			STTrait *trait = (STTrait *)item;
			method = findMethodinMethodDictionary(methodName, (STDictionary *)detagObject(trait->methodDict) );
			if( method != (STCompiledMethod *)detagObject(nil) )
				return method;
			if(trait->traitComposition != nil ) {
				method = findMethodinTraitComposition(methodName, (STTraitComposition *)detagObject(trait->traitComposition) );
				if( method != (STCompiledMethod *)detagObject(nil) )
					return method;
			}
			continue;
		}
		error("Unkonw Trait type");
	}
	return((STCompiledMethod *)detagObject(nil));
}

extern CObjectPointer ProtoObject_doesNotUnderstandC58;
extern CObjectPointer Dictionary;
STCompiledMethod *lookupMethodInClass( STClass *oClass, STString* selector) {
  STClass *cls = oClass;
  STCompiledMethod *method;
  while( cls != (STClass *)detagObject(nil) ) {
//	  if(getClass(cls->methods)!=Dictionary)
//		  print(tagObject(cls));
	  method = findMethodinMethodDictionary(selector, (STDictionary *)detagObject(cls->methods) );
	  if( method != (STCompiledMethod *)detagObject(nil) )
		  break;
	  if(cls->traitComposition != nil ) {
		  method = findMethodinTraitComposition(selector, (STTraitComposition *)detagObject(cls->traitComposition) );
		  if( method != (STCompiledMethod *)detagObject(nil) )
			  break;
	  }
	  cls = (STClass *)detagObject(cls->superClass);
  };
/*
  if( method == (STCompiledMethod *)detagObject(nil) && cls == (STClass *)detagObject(nil) ) {
 	 method = (STCompiledMethod *)detagObject(ProtoObject_doesNotUnderstandC58);

	  printf("Selector :");
	  print(tagObject((STObject *)selector));
	  printf("Class :");
      print(tagObject((STObject *)oClass));
	  error("Message not understood");
  }
*/
   return method;
}

typedef struct {
    STClass *cls;
    STString *selector;
	CFunctionPointer functionPointer;
}MethodCache;

#define MethodCacheSize 256
#define MethodCacheMask MethodCacheSize - 4
#define CacheProbeMax 4

__thread MethodCache methodCache[MethodCacheSize];

void initializeMethodCache( void ) {
    int i;
    for(i=0; i<MethodCacheSize; i++)
		methodCache[i].selector = (STString *)detagObject(nil);
}

void addFunctionPointerToCache(STClass *oClass,STString* selector, CFunctionPointer functionPointer ) {
    int hash = (int)selector;
    hash ^= (int)oClass;
	hash &= MethodCacheMask;
	int probe = hash;
	int p;
    for( p=0; p<CacheProbeMax; p++ ) {
		if(methodCache[probe].selector == (STString *)detagObject(nil)) {
			methodCache[probe].cls = oClass;
			methodCache[probe].selector = selector;
			methodCache[probe].functionPointer = functionPointer;
			return;
		}
		probe ++;
	}
	probe = hash;
	methodCache[probe].cls = oClass;
	methodCache[probe].selector = selector;
	methodCache[probe].functionPointer = functionPointer;
	probe ++;
	for( p=1; p<CacheProbeMax; p++ ) {
		methodCache[probe].selector = (STString *)detagObject(nil);
		probe++;
    }
}

void deleteSelectorFromMethodCache(STString* selector) {
    int p;
    for( p=0; p<MethodCacheSize; p++ )
        if( methodCache[p].selector == selector ) {
			methodCache[p].selector = (STString *)detagObject(nil);
		}
}

static inline
CFunctionPointer findFunctionInMethodCache(STClass *cls, STString *selector) {
	int probe;
	MethodCache *cacheLine;
	int hash = (((int)selector)^((int)cls))&MethodCacheMask;
	//First try
	cacheLine = &methodCache[hash];
	if(cacheLine->selector == selector && cacheLine->cls == cls )
		return( cacheLine->functionPointer );
	
	//Second try
	hash++;
	cacheLine = &methodCache[hash];
	if(cacheLine->selector == selector && cacheLine->cls == cls )
		return( cacheLine->functionPointer );
	
	//Third try
	hash++;
	cacheLine = &methodCache[hash];
	if(cacheLine->selector == selector && cacheLine->cls == cls )
		return( cacheLine->functionPointer );
	
	//Fourth try
	hash++;
	cacheLine = &methodCache[hash];
	if(cacheLine->selector == selector && cacheLine->cls == cls )
		return( cacheLine->functionPointer );
	
	return (CFunctionPointer)generalError;
}

CObjectPointer showMessageSends = generalError;
static inline
CFunctionPointer findFunction(STClass *cls, STString *selector) {
#ifdef DEBUG
	sendCount ++;
/*
	if( sendCount > 15 )
		error("Too many sends");

	showMessageSends = true;
*/	if( showMessageSends == true ) {
		printf("Send number: %i, ", sendCount);
		printString(getClassName(tagObject((STObject *)cls)));
		printf("->");
		printString(tagObject((STObject *)selector));
		printf("\n");
	}
#endif

    // begin searching for method
    // first see if it is in cache
	CFunctionPointer functionPointer = findFunctionInMethodCache( cls, selector );
	
    // if it is not on cache, search for it the hard way
    // and add method to cache
	if( functionPointer == (CFunctionPointer)generalError ) {
		STCompiledMethod *method = lookupMethodInClass( cls, selector );
		if( method == (STCompiledMethod *)detagObject(nil) )
			functionPointer = (CFunctionPointer)generalError;
		else {
			STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
			functionPointer = (CFunctionPointer)bytecodes->array;
			addFunctionPointerToCache( cls, selector, functionPointer );
		}
/*
		if( sendCount >= 0 ) {
			initializeMethodCache();
			printf("Send number: %i\n", sendCount);
			printString(method->source);
			printf("\n");
		}
*/
	}
	return functionPointer;
}

typedef struct  {
	STHeader header;
	CObjectPointer selector;
	CObjectPointer args;
	CObjectPointer lookupClass;
}STMessage;

extern CObjectPointer Message;
extern CObjectPointer Array;
CObjectPointer doesNotUnderstand( CObjectPointer selector, CObjectPointer receiver ) {
	STCompiledMethod *method = (STCompiledMethod *)detagObject(ProtoObject_doesNotUnderstandC58);
	STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
	CFunctionPointer function = (CFunctionPointer)bytecodes->array;
	CObjectPointer newMessage = instantiateClass( Message );
	STMessage *message = (STMessage *)detagObject( newMessage );
	message->selector = selector;
	message->args = newObject( Array, newInteger(0) );
#ifdef DEBUG
	print(selector);
	print(receiver);
	error("DNU");
#endif
	CObjectPointer returnValue = function( receiver, newMessage );
	error("Returning from DNU");
}

__attribute__((regparm(1)))
		CFunctionPointer getMethodIP( CObjectPointer selector, CObjectPointer receiver ) {
	CFunctionPointer function = findFunction( (STClass *)detagObject(getClass( receiver )), (STString *)detagObject(selector) );
/*	
	if( sendCount >= 0 ) {
		printf("Send number: %i, ", sendCount);
		printString(getClassName(getClass( receiver )));
		printf("->");
		printString(selector);
		printf("\n");
//		print( receiver );
	}
*/
	if( function == (CFunctionPointer)generalError )
		doesNotUnderstand( selector, receiver );
	return function;
}

__attribute__((regparm(2)))
		CFunctionPointer getSuperMethodIP( CObjectPointer selector, CObjectPointer cls ) {
	STClass *class = (STClass *)detagObject(cls);
	STClass *superClass = (STClass *)detagObject(class->superClass);
	CFunctionPointer function =  findFunction( superClass, (STString *)detagObject(selector) );
	if( function == (CFunctionPointer)generalError )
		error("DNU in super send");
	return function;
}
