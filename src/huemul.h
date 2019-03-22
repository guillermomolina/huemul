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

#ifndef __huemul__
#define __huemul__

#ifdef DEBUG
#define MaxMemorySize	256*1024*1024	/* in bytes */
#else
#define MaxMemorySize	1024*1024*1024 /* in bytes */
#endif

#define MaxObjects	MaxMemorySize/100
#define MaxObjectSize MaxMemorySize/1024

typedef unsigned int CObjectPointer;
typedef CObjectPointer(*CFunctionPointer) ();


typedef struct {
	unsigned int dummy:1; //Always 0 for smallInts!!!
	int value:31;
}STInteger;

typedef enum{
	FixedObject,
	VariableObject,
	VariableByteObject,
	VariableWordObject
} objectType;

typedef struct {
	unsigned int dummy:1; //Always 0 for smallInts!!!
	unsigned int hash:12;
	unsigned int type:2;
	unsigned int weak:1;
	unsigned int unused:15;
	unsigned int marked:1;
}STHeaderInfo;

typedef struct{
	CObjectPointer class;
	STInteger *size;
	STHeaderInfo info;
}STHeader;

typedef struct{
	STHeader header;
}STObject;

typedef struct{ //String or Symbol!!!
	STHeader header;
	unsigned char string[];
}STString;

typedef struct{
	STHeader header;
	double value;
}STFloat;

typedef struct{
	STHeader header;
	CObjectPointer array[];
}STArray;

typedef struct{
	STHeader header;
	STInteger *tally;
	CObjectPointer array;
} STDictionary;

typedef struct{
	STHeader header;
	STInteger *tally;
	CObjectPointer array;
	CObjectPointer cachedClassNames;
} STSystemDictionary;

typedef struct {
	unsigned int dummy:1; //Always 0 for smallInts!!!
	unsigned int type:2;
	unsigned int weak:1;
	unsigned int unused:28;
}STClassFormat;

typedef struct {
	STHeader header;
	CObjectPointer superClass;
	CObjectPointer methods;
	STInteger *instanceSize;
	STClassFormat format;
	CObjectPointer traitComposition;
	CObjectPointer variables;
	CObjectPointer organization;
	CObjectPointer subclasses;
	CObjectPointer name;
	CObjectPointer classPool;
	CObjectPointer sharedPools;
	CObjectPointer environment;
	CObjectPointer category;
}STClass;

typedef struct {
	STHeader header;
	CObjectPointer superClass;
	CObjectPointer methods;
	STInteger *instanceSize;
	STClassFormat format;
	CObjectPointer traitComposition;
	CObjectPointer variables;
	CObjectPointer organization;
	CObjectPointer thisClass;
}STMetaclass;

typedef struct  {
	STHeader header;
	CObjectPointer bytecodes;
	CObjectPointer literals;
	STInteger *numArgs;
	STInteger *numTemps;
	CObjectPointer relocationInfo;
	CObjectPointer owner;
	CObjectPointer debugInfo;
	STInteger *primitive;
	CObjectPointer source;
}STCompiledMethod;

typedef struct  {
	STHeader header;
	CObjectPointer key;
	CObjectPointer value;
}STAssociation;

typedef struct {
	STHeader header;
	STInteger *value;
}STCharacter;

typedef struct {
	STHeader header;
	CObjectPointer method;
	CObjectPointer environment;
}STBlockClosure;

typedef struct {
	STHeader header;
	CObjectPointer collection;
	STInteger *position;
	STInteger *readLimit;
	STInteger *writeLimit; //Only available in WriteStreams
}STStream;

typedef struct {
	STHeader header;
	CObjectPointer methodDict;
	CObjectPointer traitComposition;
	CObjectPointer users;
	CObjectPointer name;
	CObjectPointer organization;
	CObjectPointer environment;
	CObjectPointer classTrait;
	CObjectPointer category;
}STTrait;

typedef struct {
	STHeader header;
	CObjectPointer transformations;
}STTraitComposition;

typedef struct {
	STHeader header;
	CObjectPointer block;
	CObjectPointer name;
	STInteger *priority;
	CObjectPointer exceptionHandlerList;
	CObjectPointer state;
	CObjectPointer stackTop;
	CObjectPointer stackPointer;
	CObjectPointer threadData;
	CObjectPointer suspendSemaphore;
}STProcess;

extern CObjectPointer nil;
extern CObjectPointer true;
extern CObjectPointer false;
extern CObjectPointer SmallInteger;
extern CObjectPointer Array;
extern CObjectPointer Float;

#define isFloat(value) ( getClass(value)==Float )
#define areFloats(f1,f2) ( isFloat( f1 ) && isFloat( f2 ) )
#define isArray(value) ( getClass(value)==Array )
#define isIntegerValue(intValue) ( (intValue ^ (intValue << 1)) >= 0 )

#define newInteger(number) (STInteger *)(((int)number)<<1)
#define getValue(number) (((int)number)>>1)
#define isSmallInteger(obj) ((((int)obj)&1) == 0)
#define generalError ((CObjectPointer)1)

// atCache.c
#define AtCacheSize 0x1000
#define AtCacheMask 0xFFF

typedef struct {
	CObjectPointer array;
	int isString;
	int size;
}AtCache;

static inline
	CObjectPointer tagObject( STObject *object ) {
	return (CObjectPointer)(((unsigned int)object)+1);
};

static inline
STObject *detagObject( CObjectPointer taggedObject ) {
	return (STObject *)(((unsigned int)taggedObject)-1);
};

static inline
int areIntegers( CObjectPointer i1, CObjectPointer i2 ) {
	return ( 1 & (((int)i1) | ((int)i2) )) == 0;
};

static inline
CObjectPointer getTrueOrFalse( int aBool ) {
	if(aBool)
		return true;
	else
		return false;
};

static inline
CObjectPointer getClass( CObjectPointer obj ) {
	if( isSmallInteger( obj) )
		return SmallInteger;
	else
		return ((detagObject(obj))->header.class);
};

// Definitions for alloc
CObjectPointer objectClone( CObjectPointer src );
CObjectPointer newVariableByteObject(CObjectPointer class, STInteger *size);
CObjectPointer newVariableWordObject(CObjectPointer class, STInteger *size);
CObjectPointer newObject(CObjectPointer class, STInteger *size);
CObjectPointer instantiateClass(CObjectPointer class);
CObjectPointer newString(char buffer[]);
CObjectPointer newFloat(double value);
CObjectPointer newExternalAddress( void *pointer );

#endif
