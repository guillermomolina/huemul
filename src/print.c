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
#include <math.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>

#ifdef DEBUG
extern unsigned int sendCount;
#endif
void printCallStack(void);
__attribute__((noreturn))
void error(char *string) {
#ifdef DEBUG
	printf("(send #%d) ",sendCount);
#endif
	printf("Error: %s\n",string);
	printCallStack();
	exit(2);
}

extern CObjectPointer Metaclass;
extern CObjectPointer Symbol;
extern CObjectPointer String;
extern CObjectPointer Character;

static struct termios stored_settings;
     
void setKeypress(void)
{
	struct termios new_settings;
     
	tcgetattr(0,&stored_settings);
     
	new_settings = stored_settings;
     
	/* Disable canonical mode, and set buffer size to 1 byte */
	new_settings.c_lflag &= (~ICANON);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
     
	tcsetattr(0,TCSANOW,&new_settings);
	return;
}
     
void resetKeypress(void)
{
	tcsetattr(0,TCSANOW,&stored_settings);
	return;
}

CObjectPointer getClassName( CObjectPointer obj ) {
	STClass *cls = (STClass *)detagObject(obj);
	if(getClass(obj)==Metaclass) {
		STClass *thisClass = (STClass *)detagObject(((STMetaclass *)cls)->thisClass);
		return thisClass->name;
	}
	if(getClass(getClass(tagObject((STObject *)cls)))!=Metaclass) {
//		printf("ERROR getClassName: Not a Class, trying to get it's name\n");
		return newString("<---------NOT A CLASS--------->");
	}
	return cls->name;
}

int isString( CObjectPointer obj ) {
	CObjectPointer cls = getClass(obj);
	return cls==Symbol||cls==String;
}

void printString(CObjectPointer obj) {
	STString *string = (STString *)detagObject(obj);
	if(isString(obj)) {
		int i;
		for(i=0; i<getValue(string->header.size);i++)
			printf("%c",string->string[i]);
	}
	else {
		printf("Not a string");
	  // error("Not a String in printString");
	}
	fflush(stdout);
}

void printInteger( STInteger *number ) {
	if(isSmallInteger(number))
		printf("%d", (getValue(number)));
}

int getAndPrintInstanceVars( STObject *object, STClass *cls, int indent );

void printIndent(int indent) {
	int i;
	for( i=0; i<indent; i++)
		printf("\t");
}

void printItem(CObjectPointer obj, int indent) {
	STObject *object = detagObject(obj);
	CObjectPointer cls = getClass(obj);
	int i;
	printIndent(indent);
	if(obj==nil) {
		printf("nil\n");
		return;
	}
	if(obj==true) {
		printf("true\n");
		return;
	}
	if(obj==false) {
		printf("false\n");
		return;
	}
	if(isSmallInteger(obj)) {
		printf("%0#10X: class=SmallInteger, value=",obj);
		printInteger((STInteger*)obj);
	}
	else {
		printf("%0#10X: class=",object);
		if(cls==Metaclass) {
			printString(getClassName(((STMetaclass *)object)->thisClass));
			printf(" class");
		}
		else
			printString(getClassName(getClass(obj)));
		printf(", size=%d",getValue(object->header.size));
		printf(", hash=%d",object->header.info.hash);
		if(object->header.info.weak)
			printf(", weak");
		if(object->header.info.marked)
			printf(", marked");
		if(indent<2) {
			switch( object->header.info.type ) {
				case FixedObject:
					if(cls==Character) {
						printf(", value=%i, ascii=%c\n",
							   getValue(((STCharacter *)object)->value),
										  getValue(((STCharacter *)object)->value));
						return;
					}
					printf(", values={\n");
					getAndPrintInstanceVars( object, (STClass *)detagObject(cls), indent + 1 );
					printIndent(indent);
					printf("}");
					break;
				case VariableObject:
					printf(", values={\n");
					int s = getValue(object->header.size)/4;
					CObjectPointer big = false;
					if(s>5) {
						s=5;
						big = true;
					}
					for(i=0;i<s;i++)
						printItem( ((STArray *)object)->array[i] , indent + 1 );
					if(big==true) {
						printIndent(indent + 1);
						printf("...\n");
					}
					printIndent(indent);
					printf("}");
					break;
				case VariableByteObject:
					if(isString(obj)) {
						printf(", value=\"");
						printString(obj);
						printf("\"");
					}
					else {
						printf(", values={\n");
						printIndent(indent + 1);
						int s = getValue(object->header.size);
						CObjectPointer big = false;
						if(s>256) {
							s=10;
							big = true;
						}
						for(i=0;i<s;i++)
							printf("0x%X ",((STString *)object)->string[i]);
						if(big==true)
							printf("...");
						printIndent(indent);
						printf("\n}");
					}
					break;
				case VariableWordObject:
					if(isFloat(obj)) {
						printf(", value=%f\n",((STFloat *)object)->value);
						return;
					}
					printf(", values={\n");
					printIndent(indent + 1);
					s = getValue(object->header.size)/4;
					big = false;
					if(s>10) {
						s=10;
						big = true;
					}
					for(i=0;i<s;i++)
						printf("[%i]->(%0#10X)=%u ",i, &((STArray *)object)->array[i], ((STArray *)object)->array[i]);
					if(big==true)
						printf("...");
					printIndent(indent);
					printf("\n}");
					break;
				default:
			//debugger();
					error("Bad object type in print");
			};
		}
	}
	printf("\n");
}

int getAndPrintInstanceVars( STObject *object, STClass *cls, int indent ) {
	if( cls == (STClass *)detagObject(nil) )
		return 0;
	int varNum = getAndPrintInstanceVars( object, (STClass *)detagObject(cls->superClass), indent );
	int i;
	if( cls->variables == nil )
		return varNum;
	int numberOfVars = getSize(cls->variables);
	STArray *variables = (STArray *)detagObject(cls->variables);
	for(i=varNum; i<numberOfVars+varNum;i++ ) {
		printIndent(indent);
		printString(variables->array[i-varNum]);
		printf("(%0#10X)=",&((STArray *)object)->array[i]);
		printItem(((STArray *)object)->array[i], indent );
	}
	return i;
}

CObjectPointer print(CObjectPointer obj) {
	printItem(obj, 0);
	return obj;
}

CObjectPointer getMethodNameInClassThatIncludes( CObjectPointer cls, char *caller, unsigned int *basePointer ) {
	STClass *class = (STClass *)detagObject(cls);
	if(class->methods == nil)
		return generalError;
	STArray *methodArray = (STArray *)detagObject(((STDictionary *)detagObject(class->methods))->array);
	int i;
	for(i = 0; i < getValue(methodArray->header.size)>>2 ; i++ ) {
		STAssociation *assoc = (STAssociation *)detagObject(methodArray->array[i]);
		if( assoc != (STAssociation *)detagObject(nil) ) {
			STCompiledMethod *method = (STCompiledMethod *)detagObject(assoc->value);
			STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
			char *start = (char *)bytecodes->array;
			char *end = start + getValue(bytecodes->header.size);
			if(start <= caller && end >= caller ) {
				printString( getClassName(cls) );
				printf(" -> #");
				printString( assoc->key );
				printf("\n");
				return assoc->key;
			}
		}
	}
	return generalError;
}

extern CObjectPointer Smalltalk;
CObjectPointer printMethodThatIncludes( char *caller, unsigned int *basePointer ) {
	STArray *classArray = (STArray *)detagObject(((STSystemDictionary *)detagObject(Smalltalk))->array);
	int i;
	CObjectPointer methodName;
	CObjectPointer class;
	STAssociation *assoc;
	for(i = 0; i < getValue(classArray->header.size)>>2 ; i++ ) {
		assoc = (STAssociation *)detagObject(classArray->array[i]);
		if( assoc != (STAssociation *)detagObject(nil) ) {
			class = assoc->value;
			if(getClass(getClass(class))==Metaclass) {
				methodName = getMethodNameInClassThatIncludes(class, caller, basePointer );
				if( methodName != generalError)
					break;
				methodName = getMethodNameInClassThatIncludes(getClass(class), caller, basePointer );
				if( methodName != generalError)
					break;
			}
		}
	}
	return generalError;
}

extern void *mainStackTop;
extern __thread CObjectPointer activeProcess;
void printCallStack(void){
	STProcess *process = (STProcess *)detagObject(activeProcess);
	STArray *stackTop = (STArray *)detagObject( process->stackTop );
	if( process->stackTop == nil || stackTop->array[ 0 ] == (CObjectPointer)NULL ) { //niled at startup
		printf("WARNING: Main stack top is nil\n");
		stackTop->array[ 0 ] = (CObjectPointer)mainStackTop;
	}
	unsigned int *basePointer;
	char *caller;
	
	asm(	"movl %%ebp,%[basePointer]"
	: [basePointer] "=r" (basePointer) );
	do{
		basePointer = (unsigned int *)*basePointer;
		if( basePointer > (unsigned int *)stackTop->array[ 0 ] )
			return;
		caller = (char *)*(basePointer+1);
	} while( printMethodThatIncludes( caller, basePointer ) == generalError );
	do{
		basePointer = (unsigned int *)*basePointer;
		if( basePointer > (unsigned int *)stackTop->array[ 0 ] )
			return;
		caller = (char *)*(basePointer+1);
	} while( printMethodThatIncludes( caller, basePointer ) /*!= generalError*/ );
}

