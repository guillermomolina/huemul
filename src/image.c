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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h> 
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>

CObjectPointer true;
CObjectPointer nil;
CObjectPointer false;
CObjectPointer Smalltalk;
CObjectPointer ClassTrait;
CObjectPointer Class;
CObjectPointer Metaclass;
CObjectPointer Trait;
CObjectPointer SmallInteger;
CObjectPointer Float;
CObjectPointer Symbol;
CObjectPointer String;
CObjectPointer Character;
CObjectPointer ProtoObject_doesNotUnderstandC58;
CObjectPointer Message;
CObjectPointer Array;
CObjectPointer ByteArray;
CObjectPointer Point;
CObjectPointer ExternalAddress;
CObjectPointer JumpBuffer;
CObjectPointer InstructionSequence;
CObjectPointer CompiledMethod;
CObjectPointer ExceptionLink;
CObjectPointer LargePositiveInteger;
CObjectPointer BlockClosure;
CObjectPointer CharacterTable;
CObjectPointer FramePointer;
CObjectPointer BlockContext;
CObjectPointer MethodContext;
CObjectPointer FunctionRelocation;
//CObjectPointer ExecutableFunctionRelocation;
CObjectPointer LibraryFunctionRelocation;
CObjectPointer PrimitiveRelocation;
CObjectPointer LiteralRelocation;
CObjectPointer SpecialSelectorRelocation;
CObjectPointer VariableRelocation;
CObjectPointer GCallback;
CObjectPointer Process;
CObjectPointer Executable;
CObjectPointer Processor;
CObjectPointer symbol_C43;
CObjectPointer symbol_C45;
CObjectPointer symbol_C42;
CObjectPointer symbol_C47;
CObjectPointer symbol_C47C47;
CObjectPointer symbol_C92C92;
CObjectPointer symbol_bitShiftC58;
CObjectPointer symbol_bitAndC58;
CObjectPointer symbol_bitOrC58;
CObjectPointer symbol_value;
CObjectPointer symbol_valueC58;
CObjectPointer symbol_C64;
CObjectPointer symbol_C61C61;
CObjectPointer symbol_C126C61;
CObjectPointer symbol_C61;
CObjectPointer symbol_C60C61;
CObjectPointer symbol_C62;
CObjectPointer symbol_C62C61;
CObjectPointer symbol_C60;
CObjectPointer symbol_class;
CObjectPointer symbol_getMethodIP;
CObjectPointer symbol_getSuperMethodIP;
CObjectPointer symbol_setjmp;
CObjectPointer symbol_kernelDisplay;
CObjectPointer symbol_initialized;
CObjectPointer symbol_running;
CObjectPointer symbol_suspended;
CObjectPointer symbol_terminated;
CObjectPointer symbol_ObjectMemorySize;
CFunctionPointer _ExceptionLink_Class_exceptionC58handlerC58_bytecodes;
CFunctionPointer _JumpBufferLink_executeEnsureHandlers_bytecodes;
CFunctionPointer _GCallback_Class_processCallbackC58_bytecodes;
CFunctionPointer _ProcessorScheduler_handleRemoteSuspend_bytecodes;
CFunctionPointer _Object_errorC58_bytecodes;

CObjectPointer getNextObject( CObjectPointer obj ) {
	STObject *object = detagObject( obj );
	int size = ((getValue(object->header.size) + 3) >> 2) + 3;
	return (CObjectPointer)(((int *)obj) + size);
}

typedef struct {
	STHeader header;
	STInteger *offset;
	CObjectPointer value;
}STRelocationInfo;

CFunctionPointer getExecutableFunctionPointer( CObjectPointer functionName ) {
	static void *libHandler = NULL;

	if(!libHandler) {
		libHandler = dlopen( NULL, RTLD_LAZY);
		if (!libHandler)
			error(dlerror());
	}
	
	char cSymbolName[1001];
	STString *name = (STString *)detagObject(functionName);
	int symbolNameSize = getSize(functionName);
	
	// copy the file name into a null-terminated C string
	if (symbolNameSize > 1000)
		error("Function name too big");

	strncpy(cSymbolName, name->string, 1000);
	cSymbolName[symbolNameSize]=0;

	CFunctionPointer libFunction = (CFunctionPointer)dlsym( libHandler, cSymbolName );
	
	char *errorString;
	if ((errorString = dlerror()) != NULL)
		error(errorString);
	
	return libFunction;
}

unsigned int relocateFunction( CFunctionPointer function , void *relativeTo ) {
	unsigned int address = (unsigned int)function;
	address -= (unsigned int) relativeTo;
	address -= 4;
	return address;
}

CFunctionPointer getFunctionPointer( CObjectPointer functionN );
CFunctionPointer getPrimitivePointer( STInteger *primitiveNumber );
__attribute__((regparm(1))) CFunctionPointer _getPrimitiveIP( STInteger *primitiveNumber );
CObjectPointer relocateMethodWith( CObjectPointer method, CObjectPointer relocation ) {
	STCompiledMethod *compiledMethod = (STCompiledMethod *)detagObject(method);
	CObjectPointer type = getClass(relocation);
	STArray *bytecodes = (STArray *)detagObject(compiledMethod->bytecodes);
	STRelocationInfo *relocationInfo = (STRelocationInfo *)detagObject(relocation);
	char *address = (char *)bytecodes->array;
	address += getValue(relocationInfo->offset);
	if(type == LiteralRelocation ) {
		*((unsigned int *)address) = (unsigned int)compiledMethod->literals;
		return true;
	}
	if(type == VariableRelocation ) {
		*((unsigned int *)address) = (unsigned int)relocationInfo->value;
		return true;
	}
	if(type == FunctionRelocation ) {
		*((unsigned int *)address) = relocateFunction(getFunctionPointer( relocationInfo->value ), address );
		return true;
	}
/*	if(type == ExecutableFunctionRelocation ) {
		*((unsigned int *)address) = relocateFunction(getFunctionPointer( relocationInfo->value ), address );
		return true;
	}
*/	if(type == PrimitiveRelocation ) {
		*((unsigned int *)address) = relocateFunction(_getPrimitiveIP( (STInteger *)relocationInfo->value ), address );
		return true;
	}
	if(type == SpecialSelectorRelocation ) {
		*((unsigned int *)address) = (unsigned int)relocationInfo->value;
		return true;
	}
	error("Error: Unknown relocation type\n");
	return generalError;
}

CObjectPointer relocateMethod( CObjectPointer method ) {
	STCompiledMethod *compiledMethod = (STCompiledMethod *)detagObject(method);
	if(compiledMethod->relocationInfo != nil) {
		STArray *relocationInfo = (STArray *)detagObject(compiledMethod->relocationInfo);
		int i;
		for(i=0; i<getSize(compiledMethod->relocationInfo); i++) {
			if( relocateMethodWith( method, relocationInfo->array[i] ) == generalError )
				return generalError;
		}
	}
	return method;
}

CObjectPointer findClass( char *className ) {
	STArray *classArray = (STArray *)detagObject(((STDictionary *)detagObject(Smalltalk))->array);
	int i;
	int classNameSize = strlen(className);
	for(i = 0; i < getValue(classArray->header.size)>>2 ; i++ )
		if( classArray->array[i] != nil ) {
			STAssociation *assoc = (STAssociation *)detagObject(classArray->array[i]);
			if( getSize(assoc->key) == classNameSize &&
						 strncmp( className, ((STString *)detagObject(assoc->key))->string , classNameSize ) == 0 )
				return assoc->value;
		}
	printf("%s\n", className);
	error("Class not found");
	return generalError;
}

CObjectPointer findSymbol( char* symbolName ) {
	static STArray *SymbolTableArray = NULL;
	int i;
	if( SymbolTableArray == NULL ) {
		STArray *poolArray = (STArray *)detagObject(((STDictionary *)detagObject(((STClass *)detagObject(Symbol))->classPool))->array);
		for(i = 0; i < getValue(poolArray->header.size)>>2 ; i++)
			if( poolArray->array[i] != nil ) {
			STAssociation *assoc = (STAssociation *)detagObject(poolArray->array[i]);
			int size = strlen("SymbolTable");
			if( getSize(assoc->key) == size &&
						 strncmp( "SymbolTable", ((STString *)detagObject(assoc->key))->string , size ) == 0 )
				SymbolTableArray =  (STArray *)detagObject(((STArray *)detagObject(assoc->value))->array[1]);
			};
		if(SymbolTableArray == NULL)
			error("Cant find SymbolTable");
	}
	for(i = 0; i < getValue(SymbolTableArray->header.size)>>2 ; i++ )
		if( getClass(SymbolTableArray->array[i]) == Symbol ) {
		STString *symbol = (STString *)detagObject(SymbolTableArray->array[i]);
		int size = strlen(symbolName);
		if( getSize(SymbolTableArray->array[i]) == size &&
				  strncmp( symbolName, symbol->string , size ) == 0 )
			return tagObject((STObject *)symbol);
		}
	printf("%s\n", symbolName);
	error("Symbol not found");
	return generalError;
}

STCompiledMethod *lookupMethodInClass( STClass *oClass, STString* selector);
CObjectPointer findMethod( CObjectPointer class, char* methodName ) {
	CObjectPointer method = tagObject( (STObject *)
			lookupMethodInClass( (STClass *)detagObject(class), (STString *)detagObject(findSymbol(methodName) ) ) );
	if( method == nil ) {
		printf("Method %s not found", methodName);
		error("Method not found");
	}
	return method;
}

CFunctionPointer findMethodBytecodes( CObjectPointer class, char* methodName ) {
	CObjectPointer method = findMethod( class, methodName );
	return (CFunctionPointer)((STArray *)detagObject(((STCompiledMethod *)detagObject(method))->bytecodes))->array;
}

extern STObject* startOfObjectMemory;
void relocateObject( CObjectPointer obj ) {
	STArray *object = (STArray *)detagObject( obj );
	object->header.class = (size_t)object->header.class + (size_t)startOfObjectMemory;
	int i;
	if( object->header.info.type == FixedObject || object->header.info.type == VariableObject )
		for( i = 0; i < getValue(object->header.size) >> 2; i++ )
			if(!isSmallInteger(object->array[i]))
				object->array[i] = (size_t)object->array[i] + (size_t)startOfObjectMemory;
}

CObjectPointer nextObject( CObjectPointer object );
void relocateImage() {
	// saveIndex is sorted by position
	CObjectPointer object = tagObject( startOfObjectMemory );
	while( object != generalError ) {
		relocateObject( object );
		object = nextObject( object );
	}
}

int relocateMethods() {
	CObjectPointer object = tagObject( startOfObjectMemory );
	while( object != generalError ) {
		CObjectPointer objectClass = getClass( object );
		if( objectClass == CompiledMethod || objectClass == InstructionSequence )
			if( relocateMethod( object ) == generalError )
				return 1;
		object = nextObject( object );
	}
	return 0;
}

int initializeGlobals() {
	nil = tagObject( startOfObjectMemory );
	false = nextObject( nil );
	true = nextObject( false );
	Smalltalk =nextObject( true );
	SmallInteger = findClass( "SmallInteger" );
	Float = findClass( "Float" );
	LargePositiveInteger = findClass( "LargePositiveInteger" );
	Character = findClass( "Character" );
	Point = findClass( "Point" );
	Array = findClass( "Array" );
	ByteArray = findClass( "ByteArray" );
	String = findClass( "String" );
	Symbol = findClass( "Symbol" );
	BlockClosure = findClass( "BlockClosure" );
	Metaclass = findClass( "Metaclass" );
	ExceptionLink = findClass( "ExceptionLink" );
	FramePointer = findClass( "FramePointer" );
	CompiledMethod = findClass( "CompiledMethod" );
	InstructionSequence = findClass( "InstructionSequence" );
//	ExecutableFunctionRelocation = findClass( "ExecutableFunctionRelocation" );
	FunctionRelocation = findClass( "FunctionRelocation" );
	PrimitiveRelocation = findClass( "PrimitiveRelocation" );
	LiteralRelocation = findClass( "LiteralRelocation" );
	SpecialSelectorRelocation = findClass( "SpecialSelectorRelocation" );
	VariableRelocation = findClass( "VariableRelocation" );
	ClassTrait = findClass( "ClassTrait" );
	Class = findClass( "Class" );
	Trait = findClass( "Trait" );
	Message = findClass( "Message" );
	ExternalAddress = findClass( "ExternalAddress" );
	JumpBuffer = findClass( "JumpBuffer" );
	BlockContext = findClass( "BlockContext" );
	MethodContext = findClass( "MethodContext" );
	GCallback = findClass( "GCallback" );
	Process = findClass( "Process" );
//	Executable = findClass( "Executable" );
	Processor = findClass( "Processor" );  //Processor is not a class, but it is in the SystemDictionary, find it as if it were a class
	symbol_C42 = findSymbol("*");
	symbol_C43 = findSymbol("+");
	symbol_C45 = findSymbol("-");
	symbol_C47 = findSymbol("/");
	symbol_C47C47 = findSymbol("//");
	symbol_C92C92 = findSymbol("\\\\");
	symbol_C60 = findSymbol("<");
	symbol_C60C61 = findSymbol("<=");
	symbol_C61 = findSymbol("=");
	symbol_C61C61 = findSymbol("==");
	symbol_C62 = findSymbol(">");
	symbol_C62C61 = findSymbol(">=");
	symbol_C64 = findSymbol("@");
	symbol_C126C61 = findSymbol("~=");
	symbol_bitShiftC58 = findSymbol("bitShift:");
	symbol_bitAndC58 = findSymbol("bitAnd:");
	symbol_bitOrC58 = findSymbol("bitOr:");
	symbol_value = findSymbol("value");
	symbol_valueC58 = findSymbol("value:");
	symbol_class = findSymbol("class");
	symbol_getMethodIP = findSymbol("getMethodIP");
	symbol_getSuperMethodIP = findSymbol("getSuperMethodIP");
	symbol_setjmp = findSymbol("setjmp");
	symbol_kernelDisplay = findSymbol("kernelDisplay");
	symbol_initialized = findSymbol("initialized");
	symbol_running = findSymbol("running");
	symbol_suspended = findSymbol("suspended");
	symbol_terminated = findSymbol("terminated");
	ProtoObject_doesNotUnderstandC58 = findMethod(findClass("ProtoObject"), "doesNotUnderstand:");
	_ExceptionLink_Class_exceptionC58handlerC58_bytecodes = findMethodBytecodes(getClass(findClass("ExceptionLink")), "exception:handler:");
	_GCallback_Class_processCallbackC58_bytecodes = findMethodBytecodes(getClass(findClass("GCallback")), "processCallback:");
	_JumpBufferLink_executeEnsureHandlers_bytecodes = findMethodBytecodes(findClass("JumpBufferLink"), "executeEnsureHandlers");
	_ProcessorScheduler_handleRemoteSuspend_bytecodes = findMethodBytecodes(findClass("ProcessorScheduler"), "handleRemoteSuspend");
	_Object_errorC58_bytecodes = findMethodBytecodes(findClass("Object"), "error:");
	//Look for Character table
	STArray *poolArray = (STArray *)detagObject(((STDictionary *)detagObject(((STClass *)detagObject(findClass("Character")))->classPool))->array);
	int i;
	CharacterTable =  generalError;
	for(i = 0; i < getValue(poolArray->header.size)>>2 ; i++)
		if( poolArray->array[i] != nil ) {
		STAssociation *assoc = (STAssociation *)detagObject(poolArray->array[i]);
		int size = strlen("CharacterTable");
		if( getSize(assoc->key) == size && strncmp( "CharacterTable", ((STString *)detagObject(assoc->key))->string , size ) == 0 )
			CharacterTable =  assoc->value;
		}
		if(CharacterTable == generalError)
			error("Cant find CharacterTable");
		return 0;
}

CObjectPointer allocatedObjects[MaxObjects];
int numberOfAllocatedObjects = 0;

void initializeAllocatedObjects( void ){
	numberOfAllocatedObjects = 0;
	CObjectPointer object = nil; //First Object
	while( object != generalError ) {
		allocatedObjects[numberOfAllocatedObjects++] = object;
		if(numberOfAllocatedObjects > MaxObjects )
			error("Increase MaxObjects");
		object = nextObject( object );
	}
}

int compareObjectAddresses( const void *arg1, const void *arg2) {
	CObjectPointer object1 = *(CObjectPointer *)arg1;
	CObjectPointer object2 = *(CObjectPointer *)arg2;
	return object1-object2;
}
		
extern size_t objectMemorySize;
CObjectPointer isKnownObject( CObjectPointer object ) {
	if( object == generalError )
		return false;
	if(isSmallInteger(object))
		return false;
	if( object < nil && object >= ((size_t)nil + objectMemorySize ))
		return false;
	if( bsearch( &object, allocatedObjects, numberOfAllocatedObjects, sizeof(CObjectPointer), compareObjectAddresses  ) )
		return true;
	return false;
}

extern CObjectPointer Metaclass;
CObjectPointer checkObjectHeader( CObjectPointer obj ) {
	STObject *object = detagObject( obj );
	CObjectPointer objectClass = object->header.class;
		//Check to see if first thing is a class
	if( !isKnownObject(objectClass) ) {
#ifdef DEBUG
		printf("WARNING: checkObjectHeader: Unknown object class type\n");
#endif
		return false;
	}
	CObjectPointer objectClassClass = getClass(objectClass);
	if( !isKnownObject(objectClassClass) ) {
#ifdef DEBUG
		printf("WARNING: checkObjectHeader: Unknown object class class type\n");
#endif
		return false;
	}
	if(objectClassClass!=Metaclass && getClass(objectClassClass)!=Metaclass){
#ifdef DEBUG
		printf("WARNING: checkObjectHeader: Object class is not Class type\n");
#endif
		return false;
	}
	if(!isSmallInteger(object->header.size)){
#ifdef DEBUG
		printf("WARNING: checkObjectHeader: Object size is not an integer\n");
#endif
		return false;
	}
	if(!isSmallInteger((int *)object+2)) { //info
#ifdef DEBUG
		printf("WARNING: checkObjectHeader: Object info is not an integer\n");
#endif
		return false;
	}
	STClass *class = (STClass *)detagObject(objectClass);
	if( object->header.info.type != class->format.type ) {
#ifdef DEBUG
		printf("WARNING: checkObjectHeader: Object type is not of its class type\n");
#endif
		return false;
	}
	if( object->header.info.weak != class->format.weak ) {
#ifdef DEBUG
		printf("WARNING: checkObjectHeader: Object type is not of its class type\n");
#endif
		return false;
	}
	if( object->header.info.type == FixedObject )
		if(getValue(class->instanceSize) != getValue(object->header.size)>>2 ) {
#ifdef DEBUG
			printf("WARNING: checkObjectHeader: Wrong object size\n");
#endif
			return false;
		}
	return true;
}

extern size_t usedMemorySize;
CObjectPointer checkAllMemory( void ) {
#ifdef DEBUG
	printf("Start checking %d objects:\n", numberOfAllocatedObjects);
#endif
	if( tagObject( startOfObjectMemory ) != nil ) {
#ifdef DEBUG
		printf("WARNING: checkAllMemory: nil is not the first object\n");
#endif
		return false;
	}
	static CObjectPointer UndefinedObject = generalError;
	if( UndefinedObject == generalError )
		UndefinedObject = findClass( "UndefinedObject" );
	if( getClass(nil) != UndefinedObject ) {
#ifdef DEBUG
		printf("WARNING: checkAllMemory: nil is corrupted\n");
#endif
		return false;
	}
	if( nextObject(nil) != false ) {
#ifdef DEBUG
		printf("WARNING: checkAllMemory: false is not the second object\n");
#endif
		return false;
	}
	static CObjectPointer False = generalError;
	if( False == generalError )
		False = findClass( "False" );
	if( getClass(false) != False ) {
#ifdef DEBUG
		printf("WARNING: checkAllMemory: false is corrupted\n");
#endif
		return false;
	}
	if( nextObject(false) != true ) {
#ifdef DEBUG
		printf("WARNING: checkAllMemory: true is not the third object\n");
#endif
		return false;
	}
	static CObjectPointer True = generalError;
	if( True == generalError )
		True = findClass( "True" );
	if( getClass(true) != True ) {
#ifdef DEBUG
		printf("WARNING: checkAllMemory: true is corrupted\n");
#endif
		return false;
	}
	if( nextObject(true) != Smalltalk ) {
#ifdef DEBUG
		printf("WARNING: checkAllMemory: Smalltalk is not the fourth object\n");
#endif
		return false;
	}
	static CObjectPointer SystemDictionary = generalError;
	if( SystemDictionary == generalError )
		SystemDictionary = findClass( "SystemDictionary" );
	if( getClass(Smalltalk) != SystemDictionary ) {
#ifdef DEBUG
		printf("WARNING: checkAllMemory: Smalltalk is corrupted\n");
#endif
		return false;
	}
#ifdef DEBUG
	printf("0%%");
	fflush(stdout);
#endif
	CObjectPointer ptr = tagObject( startOfObjectMemory );
	int i = 0;
	size_t size = 0;
	while( ptr != generalError ) {
#ifdef DEBUG		
		if( ((size_t)i)%20000 == 0) {
			printf("\r%d%%", i * 100 / numberOfAllocatedObjects);
			fflush(stdout);
		}
		i++;
#endif
		STArray *object = (STArray *)detagObject(ptr);
		size += roundToLong( getValue(object->header.size) ) + sizeof( STHeader );
		if( checkObjectHeader( ptr ) == false ) {
#ifdef DEBUG
			printf("WARNING: checkAllMemory: Wrong header in:\n");
			print( ptr );
#endif
			return false;
		}
/*		if( object->header.info.type == FixedObject || object->header.info.type == VariableObject ) {
			int j;
			for( j = 0; j < getValue(object->header.size) >> 2; j++ )
				if( !isSmallInteger(object->array[j]) ) {
					if( checkObjectHeader( object->array[j] ) == false ) {
#ifdef DEBUG
						printf("WARNING: checkAllMemory: Wrong header in instance of:\n");
						print( ptr );
#endif
						return false;
					}
					if( isKnownObject( object->array[j] ) == false ) {
#ifdef DEBUG
						printf("WARNING: checkAllMemory: Unknown object in instance of:\n");
						print( ptr );
#endif
						return false;
					}
				}
		}
*/		ptr = nextObject( ptr );
	}
#ifdef DEBUG
	printf("\r100%%... done\n");
#endif
	if( size != usedMemorySize )
		error( "WARNING: checkAllMemory: incorrect used memory size" );
	return true;
}

CObjectPointer loadFileIntoMemory( char* imageName );
CObjectPointer loadImage(char *imageName) {
	if( loadFileIntoMemory( imageName ) == generalError ) {
		printf("Can't load image\n" );
		return generalError;
	}
#ifdef DEBUG
	printf("Relocating image..." );
#endif
	relocateImage();
#ifdef DEBUG
	printf(" done\nInitializing globals..." );
#endif
	if (initializeGlobals()) {
		printf("Can't initialize globals\n" );
		return generalError;
	}
#ifdef DEBUG
	printf(" done\nRelocating methods..." );
#endif
	if (relocateMethods()) {
		printf("Can't relocate methods\n" );
		return generalError;
	}
#ifdef DEBUG
	printf(" done\n" );
	initializeAllocatedObjects();
	if( checkAllMemory() == false )
		error("Something wrong in the image");
#endif
	return true;
}

typedef struct {
	size_t position;
	CObjectPointer object;
} Position;

#define SaveArraySize	MaxObjects
unsigned int saveIndex;
Position saveArray[SaveArraySize];

void initializeSaveArray( void ){
	saveIndex = 0;
	size_t position = 1;
	CObjectPointer object = nil; //First Object
	while( object != generalError ) {
		saveArray[saveIndex].object = object;
		saveArray[saveIndex].position = position;
		saveIndex++;
		if(saveIndex > SaveArraySize )
			error("Increase SaveArraySize");
		STObject *obj = detagObject( object );
		position += roundToLong(getValue(obj->header.size)) + sizeof(STHeader);
		object = nextObject( object );
	}
}

int positionCompare( const void *arg1, const void *arg2) {
	Position *position1 = (Position *)arg1;
	Position *position2 = (Position *)arg2;
	return (position1->object-position2->object);
}
		
int getPositionOfObject( CObjectPointer object ) {
	Position searchPosition = { 0, object };
	Position *position = (Position *)bsearch( &searchPosition, saveArray, saveIndex, sizeof(Position), &positionCompare  );
	if( !position )
		return -1;
	return position->position;
}

#ifdef DEBUG
void copyThisClassInstead( CObjectPointer src, CObjectPointer trgt, CObjectPointer copiedObj ) {
	STClass *source = (STClass *)detagObject( src );
	STArray *target = (STArray *)detagObject( trgt );
	size_t *copiedObject = (int *)detagObject( copiedObj );
	copiedObject[0] = getPositionOfObject(source->header.class);
	copiedObject[1] = ((size_t *)source)[1];
	copiedObject[2] = ((size_t *)source)[2];
	int i;
	for( i = 0; i < getValue(target->header.size) >> 2; i++ )
		if(!isSmallInteger(target->array[i]))
			copiedObject[i+3] = getPositionOfObject( target->array[i] );
		else
			copiedObject[i+3] = (size_t)target->array[i];
	copiedObject[11] = getPositionOfObject( source->name );
}
#endif


void copyObjectTo( CObjectPointer obj, CObjectPointer copiedObj ) {
/*	
#ifdef DEBUG
	static CObjectPointer class1 = generalError;
	if( class1 == generalError )
		class1 = findClass("NativeFunction");
	static CObjectPointer class2 = generalError;
	if( class2 == generalError )
		class2 = findClass("NativeFunction2");
	if( obj == class1 )
		return copyThisClassInstead( obj, class2, copiedObj );
	if( obj == class2 )
		return copyThisClassInstead( obj, class1, copiedObj );
#endif
	*/
	STArray *object = (STArray *)detagObject( obj );
	size_t *copiedObject = (int *)detagObject( copiedObj );
	copiedObject[0] = getPositionOfObject(object->header.class);
	copiedObject[1] = ((size_t *)object)[1];
	copiedObject[2] = ((size_t *)object)[2];
	int i;
	if( getClass(obj) == ExternalAddress ) { // nil out external references
		copiedObject[3] = 0;
		return;
	}
	switch( object->header.info.type ) {
		case FixedObject:
		case VariableObject:
			for( i = 0; i < getValue(object->header.size) >> 2; i++ )
				if(!isSmallInteger(object->array[i]))
					copiedObject[i+3] = getPositionOfObject( object->array[i] );
				else
					copiedObject[i+3] = (size_t)object->array[i];
			break;
		case VariableByteObject:
		case VariableWordObject:
			for( i = 0; i < (getValue(object->header.size)+3) >> 2; i++ )
				copiedObject[i+3] = (size_t)object->array[i];
			break;
		default:
			error("Bad colection");
	};
}

int copyObjectMemoryTo( CObjectPointer saveBuffer ) {
	CObjectPointer object = nil; //First Object
	CObjectPointer copiedObject = saveBuffer;
	while( object != generalError ) {
		copyObjectTo( object, copiedObject );
		STObject *obj = detagObject( object );
		copiedObject += roundToLong(getValue(obj->header.size)) + sizeof(STHeader);
		object = nextObject( object );
	}
	if( copiedObject-saveBuffer != usedMemorySize )
		return 1;
	return 0;
}

CObjectPointer saveImage( char *imageName ) {
	initializeAllocatedObjects();
#ifdef DEBUG
	if(checkAllMemory()==false)
		printf("WARNING: saveImage: Something wrong in memory before saving\n");
#endif
	initializeSaveArray();
	STObject *saveBuffer = (STObject *)malloc( usedMemorySize );
	if( saveBuffer == NULL ) {
		printf("saveImage: Can't create save buffer\n" );
		return generalError;
	}
	if( copyObjectMemoryTo( tagObject( saveBuffer ) ) < 0 ) {
		printf("saveImage: Can't copy object memory\n" );
		return generalError;
	}
	int imageFile = open (imageName, O_WRONLY | O_TRUNC | O_CREAT, 00644 );
	if( imageFile < 0 ) {
		printf("saveImage: Can't open image file: %s\n", imageName );
		return generalError;
	}
	if( write(imageFile, saveBuffer, usedMemorySize) != usedMemorySize ) {
		printf("saveImage: Can't write image file\n" );
		return generalError;
	}
	if( close(imageFile) < 0 ) {
		printf("saveImage: Can't close image file\n" );
		return generalError;
	}
	free( saveBuffer );
	return nil;
}

CObjectPointer become( CObjectPointer rcvr, CObjectPointer arg, CObjectPointer twoWays ) {
	int i;
	for( i = 0; i < numberOfAllocatedObjects; i++ ) {
		STArray *object = (STArray *)detagObject(allocatedObjects[i]);
		if( object->header.info.type == FixedObject || object->header.info.type == VariableObject ){
			int j;
			for( j = 0; j < getValue(object->header.size) >> 2; j++ ) {
				if( twoWays == true && arg == object->array[j] )
					object->array[j] = rcvr;
				else if( rcvr == object->array[j] )
					object->array[j] = arg;
			}
		}
	}
}

CObjectPointer runHuemul( int argc, char *argv[] ) {
	CObjectPointer startClass = findClass("Shell");
	CFunctionPointer entryPoint = findMethodBytecodes( getClass(startClass) , "startUpWithArguments:" ); 
	if( entryPoint == (CFunctionPointer)generalError )
		return generalError;
	CObjectPointer argArray = newObject( Array, newInteger(argc) );
	STArray *arguments = (STArray *)detagObject( argArray );
	int i;
	for( i = 0; i < argc; i++ )
		arguments->array[ i ] = newString( argv[i] );
	return entryPoint( startClass, argArray );
}
