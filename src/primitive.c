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
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <setjmp.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <dlfcn.h>

// primitive 1
CObjectPointer primitiveSmallIntegerAdd( CObjectPointer rcvr, CObjectPointer arg ) {
	if(areIntegers(rcvr,arg)) {
		int result;
		asm(	"movl $1,%%edx\n\t"
				"movl %[rcvr],%[result]\n\t"
				"addl %[arg],%[result]\n\t"
				"cmovol %%edx,%[result]"
			: [result] "=r" (result)
			: [rcvr] "r" (rcvr), [arg] "r" (arg)
			: "edx" );
		return result;
	}
	return generalError;
}

// primitive  2
CObjectPointer primitiveSmallIntegerSubstract( CObjectPointer rcvr, CObjectPointer arg ) {
	if(areIntegers(rcvr,arg)) {
		int result;
		asm(	"movl $1,%%edx\n\t"
				"movl %[rcvr],%[result]\n\t"
				"subl %[arg],%[result]\n\t"
				"cmovol %%edx,%[result]"
			: [result] "=r" (result)
			: [rcvr] "r" (rcvr), [arg] "r" (arg)
			: "edx" );
		return result;
	}
	return generalError;
}

// primitive  3
CObjectPointer primitiveSmallIntegerLess( CObjectPointer rcvr, CObjectPointer arg ) {
	/* Tricky!, we dont need to detag in order to compare */
	if(areIntegers(rcvr,arg))
		return getTrueOrFalse( ((int)rcvr) < ((int)arg) );
	return generalError;
}

// primitive  4
CObjectPointer primitiveSmallIntegerGreater( CObjectPointer rcvr, CObjectPointer arg ) {
	/* Tricky!, we dont need to detag in order to compare */
	if(areIntegers(rcvr,arg))
		return getTrueOrFalse( ((int)rcvr) > ((int)arg) );
	return generalError;
}

// primitive 5
CObjectPointer primitiveSmallIntegerLessOrEqual( CObjectPointer rcvr, CObjectPointer arg ) {
	/* Tricky!, we dont need to detag in order to compare */
	if(areIntegers(rcvr,arg))
		return getTrueOrFalse( ((int)rcvr) <= ((int)arg) );
	return generalError;
}

// primitive  6
CObjectPointer primitiveSmallIntegerGreaterOrEqual( CObjectPointer rcvr, CObjectPointer arg ) {
	/* Tricky!, we dont need to detag in order to compare */
	if(areIntegers(rcvr,arg))
		return getTrueOrFalse( ((int)rcvr) >= ((int)arg) );
	return generalError;
}

// primitive  7
extern CObjectPointer LargePositiveInteger;
CObjectPointer primitiveSmallIntegerEqual( CObjectPointer rcvr, CObjectPointer arg ) {
	/* Tricky!, we dont need to detag in order to compare */
	if(areIntegers(rcvr,arg))
		return getTrueOrFalse( ((int)rcvr) == ((int)arg) );
	
	STArray *number;
	unsigned int receiver;
	int intReceiver = getValue(rcvr);
	if(isSmallInteger(rcvr) && intReceiver >= 0 )
		receiver = intReceiver;
	else
		if( getClass( rcvr ) == LargePositiveInteger ) {
			number = (STArray *)detagObject(rcvr);
			if(getValue(number->header.size) > 4 )
				return generalError;
			receiver = (unsigned int)number->array[0];
		}
		else
			return generalError;
	unsigned int argument;
	int intArg = getValue(arg);
	if(isSmallInteger(arg) && intArg >= 0 )
		argument = intArg;
	else
		if( getClass( arg ) == LargePositiveInteger ) {
			number = (STArray *)detagObject(arg);
			if(getValue(number->header.size) > 4 )
				return generalError;
			argument = (unsigned int)number->array[0];
		}
		else
			return generalError;
	return getTrueOrFalse( receiver == argument );
}

// primitive  8
CObjectPointer primitiveSmallIntegerNotEqual( CObjectPointer rcvr, CObjectPointer arg ) {
	/* Tricky!, we dont need to detag in order to compare */
	if(areIntegers(rcvr,arg))
		return getTrueOrFalse( ((int)rcvr) != ((int)arg) );
	return generalError;
}

// primitive  9
CObjectPointer primitiveSmallIntegerMultiply( CObjectPointer rcvr, CObjectPointer arg ) {
	if(areIntegers(rcvr,arg)) {
		int receiver = getValue(rcvr);
		int argument = getValue(arg);
		int result = receiver * argument;
		if( ((argument == 0) || (result / argument == receiver)) && isIntegerValue(result) )
			return (CObjectPointer)newInteger( result );
	}
	return generalError;
}

// primitive  10
CObjectPointer primitiveSmallIntegerDivide( CObjectPointer rcvr, CObjectPointer argu ) {
	if(!areIntegers(rcvr,argu))
		return generalError;
	int receiver = getValue(rcvr);
	int arg = getValue(argu);
	if(!arg)
		return generalError;
	if((receiver%arg)!=0)
		return generalError;
	int result = receiver/arg;
	return((CObjectPointer)newInteger(result));
}

// primitive  11
CObjectPointer primitiveSmallIntegerMod( CObjectPointer rcvr, CObjectPointer argu ) {
	if(!areIntegers(rcvr,argu))
		return generalError;
	int receiver = getValue(rcvr);
	int arg = getValue(argu);
	if(!arg)
		return generalError;
	int result = receiver%arg;
	if(arg<0) {
		if(result>0)
			result += arg;
	}
	else
		if(result<0)
			result += arg;
	return((CObjectPointer)newInteger(result));
}

// primitive  12
CObjectPointer primitiveSmallIntegerDiv( CObjectPointer rcvr, CObjectPointer argu ) {
	if(!areIntegers(rcvr,argu))
		return generalError;
	int receiver = getValue(rcvr);
	int arg = getValue(argu);
	if(!arg)
		return generalError;
	int result;
	if(receiver>0)
		if(arg>0)
			result = receiver/arg;
		else
			result = -((receiver + (-arg - 1)) / -arg);
	else
		if(arg>0)
			result = -((-receiver + (arg - 1)) / arg);
		else
			result = -receiver/-arg;
	return((CObjectPointer)newInteger(result));
}

// primitive  13
CObjectPointer primitiveSmallIntegerQuo( CObjectPointer rcvr, CObjectPointer argu ) {
	if(!areIntegers(rcvr,argu))
		return generalError;
	int receiver = getValue(rcvr);
	int arg = getValue(argu);
	if(!arg)
		return generalError;
	int result;
	if(receiver>0)
		if(arg>0)
			result = receiver/arg;
		else 
			result = -((receiver + (-arg - 1)) / -arg);
	else
		if(arg>0)
			result = -((-receiver + (arg - 1)) / arg);
		else
			result = -receiver/-arg;
	return((CObjectPointer)newInteger(result));
}

// primitive  14
CObjectPointer primitiveSmallIntegerBitAnd( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areIntegers(rcvr,arg) )
		return (CObjectPointer)( ((int)rcvr) & ((int)arg) );
	return generalError;
}

// primitive  15
CObjectPointer primitiveSmallIntegerBitOr( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areIntegers(rcvr,arg) )
		return (CObjectPointer)( ((int)rcvr) | ((int)arg) );
	return generalError;
}

// primitive  16
CObjectPointer primitiveSmallIntegerBitXor( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areIntegers(rcvr,arg) )
		return (CObjectPointer)( ((int)rcvr) ^ ((int)arg) );
	return generalError;
}

// primitive  17
CObjectPointer primitiveSmallIntegerBitShift( CObjectPointer rcvr, CObjectPointer arg ) {
	if( !areIntegers(rcvr,arg) )
		return generalError;
	STInteger *result;
	if(getValue(arg)<0) {
		if( getValue(arg) < -31 )
			return generalError;
		result = newInteger(getValue(rcvr)>>(-getValue(arg)));
	}
	else {
		if( getValue(arg) > 31 )
			return generalError;
		int shifted = getValue(rcvr)<<getValue(arg);
		if( !isIntegerValue(shifted) )
			return generalError;
		if( ( shifted >> getValue(arg) ) != getValue(rcvr) )
			return generalError;
		result = newInteger(shifted);
	}
	return((CObjectPointer)result);
}

// primitive  18
extern CObjectPointer Point;
extern CObjectPointer Float;
CObjectPointer primitiveMakePoint( CObjectPointer rcvr, CObjectPointer arg ) {
	if( ( isSmallInteger( rcvr ) || isFloat(rcvr) ) && ( isSmallInteger( arg ) || isFloat(arg) ) ) {
		CObjectPointer newPoint = instantiateClass( Point );
		STArray *point = (STArray *)detagObject( newPoint );
		point->array[ 0 ] = rcvr;
		point->array[ 1 ] = arg;
		return newPoint;		
	}
	return generalError;
}

// primitive  40
CObjectPointer primitiveSmallIntegerAsFloat( CObjectPointer rcvr ) {
	if( !isSmallInteger( rcvr ) )
		return generalError;
	return newFloat( (double)getValue( rcvr ) );
}

// primitive 41
CObjectPointer primitiveFloatAdd( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return newFloat( receiver->value + argument->value );
	}
	return generalError;
}

// primitive 42
CObjectPointer primitiveFloatSubstract( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return newFloat( receiver->value - argument->value );
	}
	return generalError;
}

// primitive  43
CObjectPointer primitiveFloatLess( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return getTrueOrFalse( receiver->value < argument->value );
	}
	return generalError;
}

// primitive  44
CObjectPointer primitiveFloatGreater( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return getTrueOrFalse( receiver->value > argument->value );
	}
	return generalError;
}

// primitive 45
CObjectPointer primitiveFloatLessOrEqual( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return getTrueOrFalse( receiver->value <= argument->value );
	}
	return generalError;
}

// primitive  46
CObjectPointer primitiveFloatGreaterOrEqual( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return getTrueOrFalse( receiver->value >= argument->value );
	}
	return generalError;
}

// primitive  47
CObjectPointer primitiveFloatEqual( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return getTrueOrFalse( receiver->value == argument->value );
	}
	return generalError;
}

// primitive  48
CObjectPointer primitiveFloatNotEqual( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return getTrueOrFalse( receiver->value != argument->value );
	}
	return generalError;
}

// primitive  49
CObjectPointer primitiveFloatMultiply( CObjectPointer rcvr, CObjectPointer arg ) {
	if( areFloats( rcvr, arg ) ) {
		STFloat *receiver = (STFloat *)detagObject(rcvr);
		STFloat *argument = (STFloat *)detagObject(arg);
		return newFloat( receiver->value * argument->value );
	}
	return generalError;
}

// primitive  50
CObjectPointer primitiveFloatDivide( CObjectPointer rcvr, CObjectPointer arg ) {
	if(!areFloats(rcvr,arg))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	STFloat *argument = (STFloat *)detagObject(arg);
	if(argument->value==0.0)
		return generalError;
	return newFloat(receiver->value/argument->value);
}

// primitive  51
CObjectPointer primitiveFloatTruncated( CObjectPointer rcvr ) {
	if(!isFloat(rcvr))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	double trunc;
	double frac = modf(receiver->value, &trunc);
	if( (-1073741824.0 <= trunc) && (trunc <= 1073741823.0) )
		return (CObjectPointer) newInteger( (int)trunc );
	return generalError;
}

// primitive  52
CObjectPointer primitiveFloatFractionalPart( CObjectPointer rcvr ) {
	if(!isFloat(rcvr))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	double trunc;
	double frac = modf(receiver->value, &trunc);
	return newFloat(frac);
}

// primitive  53
CObjectPointer primitiveFloatExponent( CObjectPointer rcvr ) {
	if(!isFloat(rcvr))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	int pwr;
	frexp(receiver->value, &pwr);
	return((CObjectPointer)newInteger(pwr-1));
}

// primitive  54
CObjectPointer primitiveFloatTimesTwoPower( CObjectPointer rcvr, CObjectPointer arg ) {
	if(!areFloats(rcvr,arg))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	int argument = getValue(arg);
	return newFloat(ldexp(receiver->value, argument));
}


// primitive  55
CObjectPointer primitiveFloatSquareRoot( CObjectPointer rcvr ) {
	if(!isFloat(rcvr))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	return newFloat(sqrt(receiver->value));
}

// primitive  56
CObjectPointer primitiveFloatSine( CObjectPointer rcvr ) {
	if(!isFloat(rcvr))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	return newFloat(sin(receiver->value));
}

// primitive  57
CObjectPointer primitiveFloatArcTan( void ) {
	error("primitiveFloatArcTan");
}

// primitive  58
CObjectPointer primitiveFloatLogN( CObjectPointer rcvr ) {
	if(!isFloat(rcvr))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	return newFloat(log(receiver->value));
}

// primitive  59
CObjectPointer primitiveFloatExp( CObjectPointer rcvr ) {
	if(!isFloat(rcvr))
		return generalError;
	STFloat *receiver = (STFloat *)detagObject(rcvr);
	return newFloat(exp(receiver->value));
}


CObjectPointer newVariableByteObject(CObjectPointer class, STInteger *size);
inline
CObjectPointer at( CObjectPointer arry, CObjectPointer indx ) {
	if( !isSmallInteger(indx) )
		return generalError;
	int index = getValue( indx );
	if( index < 1 )
		return generalError;
	STArray *array = (STArray *)detagObject( arry );
	int arraySize = getValue( array->header.size );
	switch( array->header.info.type ) {
		case VariableObject:
			if( index > arraySize >> 2 )
				return generalError;
			return array->array[ index - 1 ];
		case VariableByteObject:
			if( index > arraySize )
				return generalError;
			unsigned char dummy = ((STString *)array)->string[ index - 1 ];
			return (CObjectPointer)newInteger( dummy );
		case VariableWordObject:
			if( index > arraySize >> 2 )
				return generalError;
			unsigned int longVal = (unsigned int) array->array[ index - 1 ];
			if( longVal < 0x40000000 )
				return (CObjectPointer)newInteger( longVal );
			CObjectPointer longObjectPointer = newVariableByteObject( LargePositiveInteger, newInteger( 4 ) );
			STArray *longObj = (STArray *)detagObject( longObjectPointer );
			longObj->array[0] = (CObjectPointer)longVal;
			return longObjectPointer;
	}
	return generalError;
}

extern CObjectPointer Character;
inline
CObjectPointer atPut( CObjectPointer arry, CObjectPointer indx, CObjectPointer val ) {
	if( !isSmallInteger(indx) )
		return generalError;
	int index = getValue( indx );
	if( index < 1 )
		return generalError;
	STArray *array = (STArray *)detagObject( arry );
	int arraySize = getValue( array->header.size );
	switch( array->header.info.type ) {
		case VariableObject:
			if( index > arraySize >> 2 )
				return generalError;
			return array->array[ index - 1 ] = val;
		case VariableByteObject:
			if( index > arraySize )
				return generalError;
			CObjectPointer byteValue = val;
			if ( getClass(val) == Character )
				byteValue = (CObjectPointer)((STCharacter *)detagObject(val))->value;
			if( !isSmallInteger(byteValue) )
				return generalError;
			int value = getValue( byteValue );
			if( value < 0 || value > 255 )
				return generalError;
			((STString *)array)->string[ index - 1 ] = value;
			return val;
		case VariableWordObject:
			if( index > arraySize >> 2 )
				return generalError;
			if( isSmallInteger(val) )
				array->array[ index - 1 ] = (CObjectPointer)getValue(val);
			else {
				STArray *longObj = (STArray *)detagObject(val);
				array->array[ index - 1 ] = (CObjectPointer)longObj->array[0];
			}
			return val;
	}
	return generalError;
}

// primitive  60
CObjectPointer primitiveObjectAt( CObjectPointer array, CObjectPointer index ) {
	return at( array, index );
}

// primitive  61
CObjectPointer primitiveObjectAtPut( CObjectPointer array, CObjectPointer index, CObjectPointer value ) {
	return atPut( array, index, value );
}

// primitive  62
CObjectPointer primitiveObjectSize( CObjectPointer rcvr ) {
	STArray *array = (STArray *)detagObject(rcvr);
	switch( array->header.info.type ) {
		case VariableObject:
			return (CObjectPointer)newInteger(getValue(array->header.size)>>2);
		case VariableByteObject:
			return (CObjectPointer)array->header.size;
		case VariableWordObject:
			return (CObjectPointer)newInteger(getValue(array->header.size)>>2);
	};
	return generalError;
}

// primitive  63
extern CObjectPointer CharacterTable;
CObjectPointer primitiveStringAt( CObjectPointer string, CObjectPointer index ) {
	CObjectPointer value = at( string, index );
	if( value == generalError )
		return generalError;
	STArray *characterTable = (STArray *)detagObject(CharacterTable);
	return characterTable->array[ getValue( value ) ];
}

// primitive  64
CObjectPointer primitiveStringAtPut( CObjectPointer string, CObjectPointer index, CObjectPointer character ) {
/*	if( getClass( value ) != Character )
		return generalError;
	CObjectPointer ascii = (CObjectPointer)((STCharacter *)detagObject(value))->value;
	CObjectPointer returnValue = atPut( string, index, ascii );
	if( returnValue != generalError )
		return value;
	return generalError; */
	return atPut( string, index, character );
}
 
// primitive  65
CObjectPointer primitiveStreamNext( CObjectPointer strm ) {
	STStream *stream = (STStream *)detagObject(strm);
	if( stream->header.info.type != VariableObject || getSize(strm) >= getValue(stream->readLimit) + 1 )
		return generalError;
	return at( strm, (CObjectPointer)stream->position );
}
	
	
// primitive  66
CObjectPointer primitiveStreamNextPut( CObjectPointer strm, CObjectPointer anObject ) {
	STStream *stream = (STStream *)detagObject(strm);
	if( stream->header.info.type != VariableObject || getSize(strm) >= getValue(stream->readLimit) + 1 )
		return generalError;
	return atPut( strm, (CObjectPointer)stream->position, anObject );
}

// primitive  67
CObjectPointer primitiveStreamAtEnd( void ) {
	error("primitiveStreamAtEnd");
}

// primitive  70
CObjectPointer isKnownObject( CObjectPointer object );
CObjectPointer primitiveBehaviorNew( CObjectPointer cls ) {
	return instantiateClass( cls );
}

// primitive  71
CObjectPointer primitiveBehaviorNewWithArg( CObjectPointer rcvr, CObjectPointer size ) {
	if( !isSmallInteger(size) )
		return generalError;
	if( getValue(size) < 0 )
		return generalError;
	STClass *cls = (STClass *)detagObject(rcvr);
	if( cls->format.type == FixedObject ) {
		error( "Bad class in #new:\n" );
		return generalError;
	}
	CObjectPointer returnValue;
	switch( cls->format.type ) {
		case VariableObject:
			returnValue = newObject( rcvr, (STInteger *)size );
			break;
		case VariableByteObject:
			returnValue = newVariableByteObject( rcvr, (STInteger *)size );
			break;
		case VariableWordObject:
			returnValue = newVariableWordObject( rcvr, (STInteger *)size );
			break;
	}
	return returnValue;
}

CObjectPointer arrayBecome( CObjectPointer rcvr, CObjectPointer arg, CObjectPointer twoWays );
// primitive  72
CObjectPointer primitiveArrayBecomeOneWay( CObjectPointer rcvr, CObjectPointer arg ) {
	return arrayBecome( rcvr, arg, false );
}

// primitive  73
CObjectPointer primitiveInstVarAt( CObjectPointer rcvr, CObjectPointer i ) {
	STClass *class = (STClass *)detagObject( getClass(rcvr) );
	int index = getValue(i) - 1;
	if(index > getValue(class->instanceSize))
		return generalError;
	STArray *receiver = (STArray *)detagObject(rcvr);
	return(receiver->array[index]);
}

// primitive  74
CObjectPointer primitiveInstVarAtPut(  CObjectPointer rcvr, CObjectPointer i, CObjectPointer value ) {
	STClass *class = (STClass *)detagObject( getClass(rcvr) );
	int index = getValue(i) - 1;
	if(index > getValue(class->instanceSize))
		return generalError;
	STArray *receiver = (STArray *)detagObject(rcvr);
	receiver->array[index] = value;
	return value;
}

// primitive  75
CObjectPointer primitiveProtoObjectIdentityHash( CObjectPointer rcvr ) {
	return (CObjectPointer)newInteger(detagObject(rcvr)->header.info.hash);
}

// primitive  77
CObjectPointer getFirstInstanceOf( CObjectPointer class );
CObjectPointer primitiveBehaviorSomeInstance( CObjectPointer class ) {
	return getFirstInstanceOf(class);
}

// primitive  78
CObjectPointer getNextInstanceAfter( CObjectPointer object );
CObjectPointer primitiveProtoObjectNextInstance( CObjectPointer object ) {
	return getNextInstanceAfter(object);
}

// primitive  81
extern CObjectPointer BlockClosure;
CObjectPointer primitiveBlockClosureValue( CObjectPointer rcvr, CObjectPointer arg1, CObjectPointer arg2, CObjectPointer arg3, CObjectPointer arg4 ) {
	if( getClass( rcvr ) != BlockClosure )
		return generalError;
	STBlockClosure *block = (STBlockClosure *)detagObject(rcvr);
	STCompiledMethod *method = (STCompiledMethod *)detagObject(block->method);
	STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
	CFunctionPointer functionPointer = (CFunctionPointer)bytecodes->array;
	switch( getValue(method->numArgs) ) {
		case 0:
			return functionPointer( block->environment );
		case 1:
			return functionPointer( block->environment, arg1 );
		case 2:
			return functionPointer( block->environment, arg1, arg2 );
		case 3:
			return functionPointer( block->environment, arg1, arg2, arg3 );		
		case 4:
			return functionPointer( block->environment, arg1, arg2, arg3, arg4 );		
	}
	return generalError; 
}

// primitive  82
CObjectPointer primitiveBlockContextValueWithArgs( void ) {
	error("primitiveBlockContextValueWithArgs");
}

// primitive  83
STCompiledMethod *lookupMethodInClass(STClass *cls, STString *selector);
CObjectPointer primitiveObjectPerform(CObjectPointer receiver, CObjectPointer selector,
		CObjectPointer arg1, CObjectPointer arg2, CObjectPointer arg3 ) {
	STCompiledMethod *method = lookupMethodInClass( (STClass *)detagObject(getClass( receiver )), (STString *)detagObject(selector) );
	STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
	CFunctionPointer functionPointer = (CFunctionPointer)bytecodes->array;
	switch( getValue( method->numArgs ) ) {
		case 0:
			return functionPointer( receiver );
		case 1:
			return functionPointer( receiver, arg1 );
		case 2:
			return functionPointer( receiver, arg1, arg2 );
		case 3:
			return functionPointer( receiver, arg1, arg2, arg3 );
	}
	return generalError;
}

// primitive  84
CObjectPointer primitiveObjectExecuteMethodWithArgs(  CObjectPointer receiver, CObjectPointer mthd, CObjectPointer args  );
CObjectPointer primitiveObjectPerformWithArgs( CObjectPointer receiver, CObjectPointer selector, CObjectPointer args ) {	
	STCompiledMethod *method = lookupMethodInClass( (STClass *)detagObject(getClass( receiver )), (STString *)detagObject(selector) );
	if( method == (STCompiledMethod *)detagObject(nil) )
		return generalError;
	return primitiveObjectExecuteMethodWithArgs( receiver, tagObject((STObject *)method), args );
}

typedef struct {
	STHeader header;
	CObjectPointer data;
}STSemaphore;

// primitive  85
CObjectPointer primitiveSemaphoreSignal( CObjectPointer sema ) {
	STSemaphore *semaphore = (STSemaphore *)detagObject( sema );
	STArray *data = ( STArray * )detagObject( semaphore->data );
	if( sem_post( (sem_t *)data->array ) < 0 )
		return generalError;
	return sema;
}

// primitive  86
extern CObjectPointer symbol_running;
extern CObjectPointer symbol_suspended;
CObjectPointer primitiveProcessWaitSemaphore(  CObjectPointer proc, CObjectPointer sema  ) {
	STProcess *process = (STProcess *)detagObject((CObjectPointer)proc);
	process->stackPointer = newExternalAddress( &proc ); // tricky, used to get base pointer
	process->state = symbol_suspended;
		
	STSemaphore *semaphore = (STSemaphore *)detagObject( sema );
	STArray *data = ( STArray * )detagObject( semaphore->data );
	if( sem_wait( (sem_t *)data->array ) < 0 ) {
		perror("primitiveSemaphoreWait");
		return generalError;
	}
	process->state = symbol_running;
	process->stackPointer = nil;
	return sema;
}

extern CFunctionPointer _ProcessorScheduler_handleRemoteSuspend_bytecodes;
extern CObjectPointer Processor;
void remoteSuspendHandler( int signal ) {
	_ProcessorScheduler_handleRemoteSuspend_bytecodes( Processor );
}

__thread CObjectPointer activeProcess;
extern CFunctionPointer _Object_errorC58_bytecodes;
void signalSegmentViolation( int signal ) {
#ifdef DEBUG
	error("Segment Violation Signal");
#endif
	_Object_errorC58_bytecodes( activeProcess, newString("Segment Violation") );
}

void initializeSignals( void ) {
	struct sigaction signal;
	
	signal.sa_flags = 0;
//	signal.sa_flags = SA_RESTART;
	signal.sa_handler = signalSegmentViolation;
	sigemptyset (&signal.sa_mask);
	if( sigaction (SIGSEGV, &signal, NULL) == -1 )
		error("Installing segment violation handler");

	signal.sa_flags = 0;
//	signal.sa_flags = SA_RESTART;
	signal.sa_handler = remoteSuspendHandler;
	sigemptyset (&signal.sa_mask);
	if( sigaction (SIGUSR1, &signal, NULL) == -1 )
		error("Installing suspend handler");
}

extern CObjectPointer symbol_terminated;
CObjectPointer value( CObjectPointer block );
void *threadRun( void *proc ) {
	// get stack top
	activeProcess = (CObjectPointer)proc;
	STProcess *process = (STProcess *)detagObject(activeProcess);
	process->stackTop = newExternalAddress( &proc );// tricky, used to get stack top
	
	process->state = symbol_running;
	if( value( process->block ) == generalError )
		error("problem running thread");
	process->state = symbol_terminated;
	pthread_exit( NULL );
	error("Am I here????");
	return NULL;
}

// primitive  88
CObjectPointer primitiveProcessRemoteSuspend( CObjectPointer proc ) {
	STProcess *process = (STProcess *)detagObject( proc );
	STArray *threadData = (STArray *)detagObject( process->threadData );
	pthread_kill( (pthread_t)threadData->array[ 0 ], SIGUSR1 );
	return proc;
}

// primitive  89
CObjectPointer primitiveObjectBehaviorFlushCache( CObjectPointer obj ) {
	// We could flush cache just for the referenced objet, but, by now, we flush everything
	initializeMethodCache();
	return obj;
}

// primitive  97
extern char *imageName;
CObjectPointer saveImage( char *imageName );
CObjectPointer primitiveSnapshot( CObjectPointer smlltlk ) {
	return saveImage( imageName );
}

// primitive  100
CObjectPointer primitiveObjectPerformWithArgsInSuper( CObjectPointer receiver, CObjectPointer selector,
														CObjectPointer args, CObjectPointer lookupClass ) {
															print(lookupClass);
															print(selector);
	STClass *rcvrClass = (STClass *)detagObject(getClass( receiver ));
	STClass *class = (STClass *)detagObject(lookupClass);
	STClass *currentClass = class;
	while(currentClass != class) {
		currentClass = (STClass *)detagObject(currentClass->superClass);
		if(currentClass == (STClass *)detagObject(nil) )
			return generalError;
	};
	STCompiledMethod *method = lookupMethodInClass( class, (STString *)detagObject(selector) );
	if( method == (STCompiledMethod *)detagObject(nil) )
		return generalError;
	return primitiveObjectExecuteMethodWithArgs( receiver, tagObject((STObject *)method), args );
}

// primitive  110
CObjectPointer primitiveObjectEquivalent( CObjectPointer rcvr, CObjectPointer arg ) {
	return(getTrueOrFalse(rcvr==arg));
}

// primitive  111
CObjectPointer primitiveObjectClass( void ) {
	error("primitiveObjectClass");
}

// primitive  112
CObjectPointer bytesLeft( void );
CObjectPointer primitiveBytesLeft( CObjectPointer smalltalk ) {
	return bytesLeft();
}

// primitive  113
CObjectPointer primitiveSystemDictionaryQuit( void ) {
	printf("Bye!\n");
#ifdef DEBUG
	error("primitiveSystemDictionaryQuit");
#endif
	exit(0);
}

// primitive  115
CObjectPointer primitiveObjectChangeClass( CObjectPointer rcvr, CObjectPointer argument ) {
/* Change the class of the receiver into the class specified by the argument given that the
	format of the receiver matches the format of the argument.
	Fail if receiver or argument are SmallIntegers, or when the format of the receiver is different
	from the format of the argument's class, or when the arguments class is fixed and the receiver's
	size differs from the size that an instance of the argument's class should have. */
	if(isSmallInteger(rcvr)||isSmallInteger(rcvr))
		return generalError;
	CObjectPointer class = getClass(argument);
	STClass *argClass = (STClass *)detagObject(class);
	STObject *receiver = detagObject(rcvr);
	if( receiver->header.info.type != argClass->format.type )
		return generalError;
	if( argClass->format.type == FixedObject && getValue(receiver->header.size)>>2 != getValue(argClass->instanceSize) )
		return generalError;
	receiver->header.class = class;
	initializeMethodCache();
	return rcvr;
}

// primitive  116
CObjectPointer primitiveCompiledMethodFlushCache( CObjectPointer compiledMethod ) {
	initializeMethodCache();
	return compiledMethod;
}

// primitive  119
void deleteSelectorFromMethodCache(STString* selector);
CObjectPointer primitiveBehaviorFlushCacheSelective( CObjectPointer selector  ) {
	deleteSelectorFromMethodCache( (STString*)detagObject(selector) );
	return selector;
}

/* primitive 123 */
CObjectPointer primitiveObjectKernelDisplay( CObjectPointer rcvr ) {
	print( rcvr );
#ifdef DEBUG
//	error("");
#endif
	return (rcvr);
}

void getMoreCore( void );
/* primitive 124 */
CObjectPointer primitiveGetMoreCore( CObjectPointer rcvr ) {
	getMoreCore();
	return (rcvr);
}

// primitive  128
CObjectPointer primitiveArrayBecome( CObjectPointer rcvr, CObjectPointer arg ) {
	return arrayBecome( rcvr, arg, true );
}

// primitive  130
CObjectPointer garbageCollect( void );
CObjectPointer primitiveGarbageCollect( CObjectPointer rcvr ) {
	return garbageCollect();
}

// primitive  131
CObjectPointer garbageCollectMost( void );
CObjectPointer primitiveGarbageCollectMost( CObjectPointer rcvr ) {
	return garbageCollectMost();
}

// primitive  132
CObjectPointer primitiveObjectPointsTo( void ) {
	error("primitiveObjectPointsTo");
}

// primitive  135
CObjectPointer primitiveTimeMillisecondClock( void ) {
	clock_t ticks = clock();
	int miliseconds = ticks * 1000 / CLOCKS_PER_SEC;
	return (CObjectPointer) newInteger(miliseconds);
}

// primitive  136
//STSemaphore *theTimerSemaphore;
//STInteger *nextWakeupTick;
CObjectPointer primitiveSignalAtMilliseconds( CObjectPointer delay, CObjectPointer sema, STInteger *milliseconds ) {
/*	if( getClass( sema ) == Semaphore ) {
		theTimerSemaphore = (STSemaphore *)detagObject( sema );
		nextWakeupTick = milliseconds;
	}
	else {
		theTimerSemaphore = (STSemaphore *)detagObject(nil);
		nextWakeupTick = newInteger( 0 );
	}
	return delay;*/
	error("primitiveSignalAtMilliseconds");
}

// primitive  137
CObjectPointer primitiveTimeSecondsClock( void ) {
	error("primitiveTimeSecondsClock");
}

// primitive  145
CObjectPointer primitiveByteArrayConstantFill( CObjectPointer rcvr, CObjectPointer value ) {
	int s;
	STArray *array = (STArray *)detagObject(rcvr);
	switch( array->header.info.type ) {
		case VariableObject:
			error("test VariableObject in prim 145");
			s=getValue(array->header.size)/4;
			int i;
			for(i=0;i<s;i++)
				array->array[i]=value;
			break;
		case VariableByteObject:
			s=getValue(array->header.size);
			int val = getValue(value);
			if(val<0||val>255)
				return generalError;
			for( i=0;i<s;i++)
				((STString *)array)->string[i]=(unsigned char)val;
			break;
		default:
			error("Bad class in #atAllPut:\n");
	};   
	return rcvr;
}

// primitive  148
CObjectPointer primitiveObjectClone( CObjectPointer rcvr ) {
	if(isSmallInteger(rcvr))
		return generalError;
	return objectClone( rcvr );
}

// primitive  150
extern CObjectPointer JumpBuffer;
CObjectPointer primitiveJumpBufferNew( CObjectPointer jumpBufferClass ) {
	return newVariableByteObject( JumpBuffer, newInteger( sizeof(jmp_buf) ) );
}

// primitive  151, primitiveJumpBufferReturn
CObjectPointer primitiveJumpBufferReturn( CObjectPointer jumpBuffer, CObjectPointer returnVal ) {
	STArray *frame = (STArray*)detagObject(jumpBuffer);
	jmp_buf *jmpBuffer = (jmp_buf *)&frame->array;
	int returnValue = (int) returnVal;
	if((int)returnValue == 0)
		returnValue = 1;
	longjmp(*jmpBuffer, returnValue);
}

// primitive  153
extern CObjectPointer ExceptionLink;
extern CFunctionPointer _ExceptionLink_Class_exceptionC58handlerC58_bytecodes;
extern CFunctionPointer _JumpBufferLink_executeEnsureHandlers_bytecodes;
CObjectPointer value( CObjectPointer rcvr );
CObjectPointer primitiveBlockClosureOnDo( CObjectPointer block, CObjectPointer exceptionType, CObjectPointer handler) {
	STArray *exceptionLink = (STArray *)
			detagObject(_ExceptionLink_Class_exceptionC58handlerC58_bytecodes( ExceptionLink, exceptionType,  handler ));
	STArray *jumpBufferArray = (STArray *)detagObject(exceptionLink->array[1]);
	jmp_buf *jmpBuffer = (jmp_buf *)&(jumpBufferArray->array);
	int returnValue = setjmp(*jmpBuffer);
	if(!returnValue) {
		returnValue = (int) value(block);
		_JumpBufferLink_executeEnsureHandlers_bytecodes(tagObject((STObject *)exceptionLink));
		return (CObjectPointer)returnValue;
	}
	if(returnValue == 1)
		returnValue = 0;	
	return (CObjectPointer)returnValue;
}

// primitive  154
CObjectPointer newIntegerOrLong( unsigned int value );
CObjectPointer primitiveRelocationInfoAddress( CObjectPointer relocationInfo ) {
	STArray *info = (STArray *)detagObject(relocationInfo);
	return newIntegerOrLong((unsigned int)info->array[1]);
}

extern CObjectPointer symbol_C43;
CObjectPointer C43();
extern CObjectPointer symbol_C45;
CObjectPointer C45();
extern CObjectPointer symbol_C42;
CObjectPointer C42();
extern CObjectPointer symbol_C47;
CObjectPointer C47();
extern CObjectPointer symbol_C47C47;
CObjectPointer C47C47();
extern CObjectPointer symbol_C92C92;
CObjectPointer C92C92();
extern CObjectPointer symbol_bitShiftC58;
CObjectPointer bitShiftC58();
extern CObjectPointer symbol_bitAndC58;
CObjectPointer bitAndC58();
extern CObjectPointer symbol_bitOrC58;
CObjectPointer bitOrC58();
extern CObjectPointer symbol_value;
CObjectPointer value();
extern CObjectPointer symbol_valueC58;
CObjectPointer valueC58();
extern CObjectPointer symbol_C64;
CObjectPointer C64();
extern CObjectPointer symbol_C61C61;
CObjectPointer C61C61();
extern CObjectPointer symbol_C126C61;
CObjectPointer C126C61();
extern CObjectPointer symbol_C61;
CObjectPointer C61();
extern CObjectPointer symbol_C60C61;
CObjectPointer C60C61();
extern CObjectPointer symbol_C62;
CObjectPointer C62();
extern CObjectPointer symbol_C62C61;
CObjectPointer C62C61();
extern CObjectPointer symbol_C60;
CObjectPointer C60();
extern CObjectPointer symbol_atC58;
CObjectPointer atC58();
extern CObjectPointer symbol_atC58putC58;
CObjectPointer atC58putC58();
extern CObjectPointer symbol_class;
CObjectPointer class();
extern CObjectPointer symbol_getMethodIP;
CFunctionPointer getMethodIP();
extern CObjectPointer symbol_getSuperMethodIP;
CFunctionPointer getSuperMethodIP();
extern CObjectPointer symbol_setjmp;
CFunctionPointer getFunctionPointer( CObjectPointer functionName ) {
	if(functionName==symbol_C43)
		return (CFunctionPointer)&C43;
	if(functionName==symbol_C45)
		return (CFunctionPointer)&C45;
	if(functionName==symbol_C42)
		return (CFunctionPointer)&C42;
	if(functionName==symbol_C47)
		return (CFunctionPointer)&C47;
	if(functionName==symbol_C47C47)
		return (CFunctionPointer)&C47C47;
	if(functionName==symbol_C92C92)
		return (CFunctionPointer)&C92C92;
	if(functionName==symbol_bitShiftC58)
		return (CFunctionPointer)&bitShiftC58;
	if(functionName==symbol_bitAndC58)
		return (CFunctionPointer)&bitAndC58;
	if(functionName==symbol_bitOrC58)
		return (CFunctionPointer)&bitOrC58;
	if(functionName==symbol_value)
		return (CFunctionPointer)&value;
	if(functionName==symbol_valueC58)
		return (CFunctionPointer)&valueC58;
	if(functionName==symbol_C64)
		return (CFunctionPointer)&C64;
	if(functionName==symbol_C61C61)
		return (CFunctionPointer)&C61C61;
	if(functionName==symbol_C126C61)
		return (CFunctionPointer)&C126C61;
	if(functionName==symbol_C61)
		return (CFunctionPointer)&C61;
	if(functionName==symbol_C60C61)
		return (CFunctionPointer)&C60C61;
	if(functionName==symbol_C62)
		return (CFunctionPointer)&C62;
	if(functionName==symbol_C62C61)
		return (CFunctionPointer)&C62C61;
	if(functionName==symbol_C60)
		return (CFunctionPointer)&C60;
	if(functionName==symbol_class)
		return (CFunctionPointer)&class;
	if(functionName==symbol_getMethodIP)
		return (CFunctionPointer)&getMethodIP;
	if(functionName==symbol_getSuperMethodIP)
		return (CFunctionPointer)&getSuperMethodIP;
	if(functionName==symbol_setjmp)
		return (CFunctionPointer)&setjmp;
	print(tagObject((STObject *)functionName));
	error("Undefined relocation in primitiveRelocationInfoAddress");
}

// primitive  155
CObjectPointer primitiveFunctionRelocationAddressRelativeTo( CObjectPointer relocationInfo, CObjectPointer bytecodes ) {
	CFunctionPointer function = (CFunctionPointer)1;
	STArray *info = (STArray *)detagObject(relocationInfo);
//	STString *functionName = (STString *)detagObject(info->array[1]);
	function = getFunctionPointer(info->array[1]);
	STArray *code = (STArray *)detagObject(bytecodes);
	function -= (int)code->array;
	function -= getValue(info->array[0]); //offset
	function -= 4;
	return newIntegerOrLong((unsigned int)function);
}

// primitive  167
CObjectPointer primitiveProcessorSchedulerYield(  CObjectPointer procesor  ) {
	sched_yield();
	return procesor;
}

CObjectPointer objectCallFunctionWithArguments( CObjectPointer receiver, CFunctionPointer functionPointer, CObjectPointer args ) {
	if( getClass(args) != Array ) {
#ifdef DEBUG
		error("Unexpected argument type");
#endif
		return generalError;
	}
	STArray *arguments = (STArray *)detagObject( args );
	int numArgs = getSize(args);
	switch( numArgs ) {
		case 0:
			return functionPointer( receiver );
		case 1:
			return functionPointer( receiver, arguments->array[0] );
		case 2:
			return functionPointer( receiver, arguments->array[0], arguments->array[1] );
		case 3:
			return functionPointer( receiver, arguments->array[0], arguments->array[1], arguments->array[2] );
		case 4:
			return functionPointer( receiver, arguments->array[0], arguments->array[1], arguments->array[2],
									arguments->array[3] );
		case 5:
			return functionPointer( receiver, arguments->array[0], arguments->array[1], arguments->array[2],
									arguments->array[3], arguments->array[4] );
		case 6:
			return functionPointer( receiver, arguments->array[0], arguments->array[1], arguments->array[2],
									arguments->array[3], arguments->array[4], arguments->array[5] );
	}
	return generalError;
}

// primitive  188
extern CObjectPointer CompiledMethod;
extern CObjectPointer InstructionSequence;
CObjectPointer primitiveObjectExecuteMethodWithArgs(  CObjectPointer receiver, CObjectPointer mthd, CObjectPointer arguments  ) {
	CObjectPointer methodClass = getClass(mthd);
	if( methodClass != CompiledMethod && methodClass != InstructionSequence ) {
#ifdef DEBUG
		error("Unexpected method class");
#endif
		return generalError;
	}
	STCompiledMethod *method = (STCompiledMethod *)detagObject(mthd);
	int numArgs = getSize(arguments);
	if( numArgs != getValue(method->numArgs) ) {
#ifdef DEBUG
		error("Unexpected argument count");
#endif
		return generalError;
	}
	STArray *bytecodes = (STArray *)detagObject( method->bytecodes );
	CFunctionPointer functionPointer = (CFunctionPointer)bytecodes->array;
	return objectCallFunctionWithArguments( receiver, functionPointer, arguments );
}

typedef struct  {
	STHeader header;
	CObjectPointer handle;
}STExternalObject;

typedef struct  {
	STHeader header;
	CObjectPointer handle;
	STInteger *size;
}STExternalBuffer;

// primitive  190
CObjectPointer primitiveExternalBufferByteAt( CObjectPointer externalBuffer, CObjectPointer indx ) {
	if( !isSmallInteger(indx) )
		return generalError;
	int index = getValue( indx );
	if( index < 1 )
		return generalError;	
	STExternalBuffer *buffer = (STExternalBuffer *)detagObject( externalBuffer );
	if( (CObjectPointer)buffer->size != nil && index > getValue(buffer->size) )
		return generalError;
	STArray *handle = (STArray *)detagObject(buffer->handle);
	unsigned char *bufferPointer = (unsigned char *)handle->array[0];
	unsigned char dummy = bufferPointer[ index - 1 ];
	return (CObjectPointer)newInteger( dummy );
}

// primitive  191
CObjectPointer primitiveExternalBufferByteAtPut( CObjectPointer externalBuffer, CObjectPointer indx, CObjectPointer val ) {
	if( !isSmallInteger(indx) )
		return generalError;
	int index = getValue( indx );
	if( index < 1 )
		return generalError;
	STExternalBuffer *buffer = (STExternalBuffer *)detagObject( externalBuffer );
	if( (CObjectPointer)buffer->size != nil && index > getValue(buffer->size) )
		return generalError;
	CObjectPointer byteValue = val;
	if ( getClass(val) == Character )
		byteValue = (CObjectPointer)((STCharacter *)detagObject(val))->value;
	if( !isSmallInteger(byteValue) )
		return generalError;
	int value = getValue( byteValue );
	if( value < 0 || value > 255 )
		return generalError;
	STArray *handle = (STArray *)detagObject(buffer->handle);
	unsigned char *bufferPointer = (unsigned char *)handle->array[0];
	bufferPointer[ index - 1 ] = value;
	return val;
}

// primitive  192
extern CObjectPointer ByteArray;
CObjectPointer primitiveSemaphoreDestroy( CObjectPointer sema );
CObjectPointer primitiveSemaphoreCreate( CObjectPointer sema ) {
	STSemaphore *semaphore = (STSemaphore *)detagObject( sema );
	if( semaphore->data != nil )
		if( primitiveSemaphoreDestroy( sema ) == generalError )
			return generalError;
	semaphore->data = newVariableByteObject( ByteArray, newInteger( sizeof( sem_t ) ) );
	STArray *data = ( STArray * )detagObject( semaphore->data );
	if( sem_init( (sem_t *)data->array, 0, 0 ) < 0 )
		return generalError;
	return sema;
}

// primitive  193
CObjectPointer primitiveSemaphoreDestroy( CObjectPointer sema ) {
	STSemaphore *semaphore = (STSemaphore *)detagObject( sema );
	STArray *data = ( STArray * )detagObject( semaphore->data );
	if( sem_destroy( (sem_t *)data->array ) < 0 )
		return generalError;
	return sema;
}

// primitive  195
CObjectPointer primitiveProcessSchedulerActiveProcess(  CObjectPointer procesor  ) {
	// Thread local storage variable
	return activeProcess;
}

// primitive  196
extern void *mainStackTop;
extern CObjectPointer Process;
CObjectPointer primitiveBeInitialProcess(  CObjectPointer process  ) {
	activeProcess = process;
	STProcess *proc = (STProcess *)detagObject(process);
	proc->stackTop = newExternalAddress(mainStackTop);
	CObjectPointer tD = newVariableByteObject( ByteArray, newInteger( sizeof( pthread_t ) ) );
	STArray *threadData = ( STArray * )detagObject( tD );
	threadData->array[ 0 ] = pthread_self();
	proc->threadData = tD;
	return process;
}

// primitive  197
CObjectPointer primitiveFunctionInvokeReceiverWithArguments( CObjectPointer funct, CObjectPointer receiver, CObjectPointer arguments ) {
	STExternalObject *function = (STExternalObject *)detagObject( funct );
	if(function->handle == nil ) //Handler
		return generalError;
	STArray *handle = (STArray *)detagObject(function->handle);
	return objectCallFunctionWithArguments( receiver, (CFunctionPointer)handle->array[0] , arguments );
}

typedef struct  {
	STHeader header;
	CObjectPointer handler;
	CObjectPointer name;
}STDynamicLibrary;

// primitive  198
extern CObjectPointer String;
extern CObjectPointer Symbol;
CObjectPointer primitiveDynamicLibraryOpen(  CObjectPointer extLib  ) {
	STDynamicLibrary *library = (STDynamicLibrary *)detagObject( extLib );
	char *cFileNamePointer = NULL;
	
	if( library->name != nil) {
		//If no name, we are opening the executable
		char cFileName[1001];
	
		if( getClass(library->name) != String && getClass(library->name) != Symbol )
			return generalError;
	
		STString *name = (STString *)detagObject(library->name);
		int fileNameSize = getSize(library->name);
	
	// copy the file name into a null-terminated C string 
		if (fileNameSize > 1000)
			return generalError;

		strncpy(cFileName, name->string, 1000);
		cFileName[fileNameSize]=0;

		cFileNamePointer = cFileName;
	}

	void *libHandler = dlopen(cFileNamePointer, RTLD_LAZY);
	if (!libHandler) {
		error(dlerror());
		return generalError;
	}
	library->handler = newExternalAddress( libHandler );
	return extLib;
}

// primitive  199
CObjectPointer primitiveBufferLoadFromByteArray( CObjectPointer anExternalBuffer, CObjectPointer byteArray ) {
	STString *array = (STString *)detagObject( byteArray );
	if( array->header.info.type != VariableByteObject )
		return generalError;
	STExternalBuffer *buffer = (STExternalBuffer *)detagObject( anExternalBuffer );
	STArray *handle = (STArray *)detagObject(buffer->handle);
	char *destination = (char *)handle->array[0];
	char *source = array->string;
	int bufferSize = getValue(buffer->size);
	int arraySize = getValue(array->header.size);
	if( bufferSize < arraySize )
		return generalError;
	int sz = bufferSize;
	if( sz > arraySize ) {
		memset(destination+arraySize, 0, bufferSize-arraySize);
		sz = arraySize;
	}
	if( memcpy(destination, source, sz) != destination )
		return generalError;
	return anExternalBuffer;
}
		
// primitive  200
CObjectPointer primitiveProcessTerminate(  CObjectPointer proc  ) {
	STProcess *process = (STProcess *)detagObject( proc );
	STArray *threadData = (STArray *)detagObject( process->threadData );
	process->stackPointer = nil;
	process->state = symbol_terminated;
	if( pthread_cancel( (pthread_t)threadData->array[ 0 ] ) ) {
		print(proc);
		error("Inexistent thread");
		return generalError;
	}
	process->threadData = nil;
	return nil;
}

typedef struct  {
	STHeader header;
	CObjectPointer handler;
	CObjectPointer name;
}STExternalLibrary;

// primitive  201
CObjectPointer primitiveExternalLibraryOpen(  CObjectPointer extLib  ) {
	STExternalLibrary *library = (STExternalLibrary *)detagObject( extLib );
	
	char *cFileNamePointer = NULL;

	static jj = 0;

	if( library->name != nil) {
		//If no name, we are opening the executable
		char cFileName[1001];
	
		if( getClass(library->name) != String && getClass(library->name) != Symbol )
			return generalError;
	
		STString *name = (STString *)detagObject(library->name);
		int fileNameSize = getSize(library->name);
	
	// copy the file name into a null-terminated C string 
		if (fileNameSize > 1000)
			return generalError;

		strncpy(cFileName, name->string, 1000);
		cFileName[fileNameSize]=0;

		cFileNamePointer = cFileName;
	}
	void *libHandler = dlopen(cFileNamePointer, RTLD_LAZY);
	if (!libHandler) {
		error(dlerror());
		return generalError;
	}
	library->handler = newExternalAddress( libHandler );
	return extLib;
}

// primitive  202
CObjectPointer primitiveExternalLibraryClose(  CObjectPointer extLib  ) {
	STExternalLibrary *library = (STExternalLibrary *)detagObject( extLib );
	if(library->handler == nil)
		return generalError;
	
	STArray *handler = (STArray *)detagObject(library->handler);

	if ( dlclose( (void *)handler->array[ 0 ] ) )
		return generalError;

	handler->array[ 0 ] = (CObjectPointer)NULL;
	return extLib;
}

typedef struct  {
	STHeader header;
	CObjectPointer handle;
	CObjectPointer name;
	CObjectPointer library;
}STExternalSymbol;

//primitive 203
CObjectPointer primitiveExternalLibraryFindFunction(  CObjectPointer extLib, CObjectPointer funct ) {
	STExternalLibrary *library = (STExternalLibrary *)detagObject( extLib );
	if(library->handler == nil)
		return generalError;
	
	STArray *handler = (STArray *)detagObject(library->handler);

	STExternalSymbol *function = (STExternalSymbol *)detagObject( funct );
	if( getClass(function->name) != String && getClass(function->name) != Symbol )
		return generalError;
	
	char cSymbolName[1001];
	STString *name = (STString *)detagObject(function->name);
	int symbolNameSize = getSize(function->name);
	
	// copy the file name into a null-terminated C string 
	if (symbolNameSize > 1000)
		return generalError;

	strncpy(cSymbolName, name->string, 1000);
	cSymbolName[symbolNameSize]=0;

	void *libFunction = dlsym( (void *)handler->array[ 0 ], cSymbolName );
	
	char *errorString;
	if ((errorString = dlerror()) != NULL)  {
		error(errorString);
		return generalError;
	}

	function->handle = newExternalAddress ( libFunction );
	return funct;
}

// primitive  204
CObjectPointer primitiveExternalFunctionInvokeWithArguments( CObjectPointer funct, CObjectPointer returnPtr, CObjectPointer args ) {
	STExternalSymbol *function = (STExternalSymbol *)detagObject( funct );
	if(function->handle == nil )
		return generalError;
	STArray *handle = (STArray *)detagObject(function->handle);
	STArray *arguments = (STArray *)detagObject(args);
	CFunctionPointer functionPointer = (CFunctionPointer)handle->array[0];
	if( functionPointer == NULL )
		return generalError;
	int arg1;
	int arg2;
	int arg3;
	int arg4;
	int arg5;
	int arg6;
	int returnValue;
	int numArgs = getSize(args);
	switch( numArgs ) {
		case 6:
			arg6 = ((STArray *)detagObject(arguments->array[5]))->array[0];
		case 5:
			arg5 = ((STArray *)detagObject(arguments->array[4]))->array[0];
		case 4:
			arg4 = ((STArray *)detagObject(arguments->array[3]))->array[0];
		case 3:
			arg3 = ((STArray *)detagObject(arguments->array[2]))->array[0];
		case 2:
			arg2 = ((STArray *)detagObject(arguments->array[1]))->array[0];
		case 1:
			arg1 = ((STArray *)detagObject(arguments->array[0]))->array[0];
	}
//	print(function->name);
/*	if(!strncmp(((STArray *)detagObject( function->name ))->array,"malloc", getSize(function->name) )) {
		print(arguments->array[0]);
		printf("malloc: %d", arg1);
//		error("Aqui\n");
	}
*/	switch( numArgs ) {
		case 0:
			returnValue = (int)functionPointer();
			break;
		case 1:
			returnValue = (int)functionPointer(arg1 );
			break;
		case 2:
			returnValue = (int)functionPointer( arg1, arg2 );
			break;
		case 3:
			returnValue = (int)functionPointer( arg1, arg2, arg3 );
			break;
		case 4:
			returnValue = (int)functionPointer( arg1, arg2, arg3, arg4 );
			break;
		case 5:
			returnValue = (int)functionPointer( arg1, arg2, arg3, arg4, arg5 );
			break;
		case 6:
			returnValue = (int)functionPointer( arg1, arg2, arg3, arg4, arg5, arg6 );
			break;
		default:
			error("Too many arguments in primitiveExternalFunctionInvokeWithArguments");
	};
	if( returnPtr != nil ) {
		STArray *returnPointer = (STArray *)detagObject( returnPtr );
		returnPointer->array[ 0 ] = (CObjectPointer)returnValue;
	}
	return returnPtr;
}

// primitive  205
CObjectPointer primitiveExternalAddressForArray( CObjectPointer extAddress, CObjectPointer arr ) {
	STArray *externalAddress = (STArray *)detagObject( extAddress );
	STArray *array = (STArray *)detagObject( arr );
	externalAddress->array[0] = (int)array->array;
	return(extAddress);
}

// primitive  206
CObjectPointer primitiveExternalAddressForObject( CObjectPointer extAddress, CObjectPointer object ) {
	STArray *externalAddress = (STArray *)detagObject( extAddress );
	externalAddress->array[0] = (int)detagObject( object );
	return(extAddress);
}

// primitive  207
CObjectPointer primitiveExternalAddressToString( CObjectPointer extAddress ) {
	STArray *externalAddress = ( STArray * )detagObject(extAddress);
	char *cString = (char *)externalAddress->array[0];
	int i;
	for( i= 0; i < strlen(cString); i++ )
		if( cString[i] == 10 )
			cString[i] = 13;
	return newString(cString);
}

// primitive  208
extern CObjectPointer ByteArray;
extern CObjectPointer symbol_initialized;
CObjectPointer primitiveProcessExecute( CObjectPointer proc  ) {
	pthread_attr_t thread_attr;
	struct sched_param thread_param;
	STProcess *process = (STProcess *)detagObject( proc );
	process->threadData = newVariableByteObject( ByteArray, newInteger( sizeof( pthread_t ) ) );
	STArray *threadData = ( STArray * )detagObject( process->threadData );
	process->state = symbol_initialized;
	pthread_attr_init (&thread_attr);
	pthread_attr_setschedpolicy (&thread_attr, SCHED_RR);
	pthread_attr_setscope(&thread_attr, PTHREAD_SCOPE_SYSTEM);
	//Linux lowest priority = 1, ST Lowest priority = 10
	//Linux lowest priority = 99, ST Lowest priority = 80
	thread_param.sched_priority = getValue( process->priority );
	pthread_attr_setschedparam (&thread_attr, &thread_param);
//	pthread_attr_setinheritsched (&thread_attr, PTHREAD_EXPLICIT_SCHED);
#define ThreadStackSize 512*1024
	if( pthread_attr_setstacksize(&thread_attr, ThreadStackSize ) != 0) {
#ifdef DEBUG
		perror(NULL);
#endif
		error("Can not set max thread stack size");
	}
	if( pthread_create((pthread_t *)threadData->array, &thread_attr, &threadRun, (void *)proc ) != 0) {
#ifdef DEBUG
		perror(NULL);
#endif
		return generalError;
	}
	static int a = 0;
	if(a==1)
		error("Aqui");
	return proc;
}

CObjectPointer findInstructionSequenceThatIncludesIPInMethodLiterals( char *caller, CObjectPointer lits) {
	if(lits == nil )
		return generalError;
	STArray *literals= (STArray *)detagObject(lits);
	int i;
	for(i = 0; i < getValue(literals->header.size)>>2 ; i++ ) {
		STCompiledMethod *method = NULL;
		if( getClass(literals->array[i]) == BlockClosure ) {
			STBlockClosure *block = (STBlockClosure *)detagObject(literals->array[i]);
			method = (STCompiledMethod *)detagObject(block->method);
		}
		if( getClass(literals->array[i]) == InstructionSequence )
			method = (STCompiledMethod *)detagObject(literals->array[i]);
		if(method) {
			STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
			char *start = (char *)bytecodes->array;
			char *end = start + getValue(bytecodes->header.size);
			if(start <= caller && end >= caller )
				return tagObject((STObject *)method);
			CObjectPointer mthd = findInstructionSequenceThatIncludesIPInMethodLiterals( caller, method->literals);
			if( mthd != generalError )
				return mthd;
		}
	}
	return generalError;
}

CObjectPointer findMethodThatIncludesIPInMethodDictionary(  char *caller, STDictionary *mDict) {
	STArray *methodArray= (STArray *)detagObject(mDict->array);
	int i;
	for(i = 0; i < getValue(methodArray->header.size)>>2 ; i++ ) {
		STAssociation *assoc = (STAssociation *)detagObject(methodArray->array[i]);
		if( assoc != (STAssociation *)detagObject(nil) ) {
			STCompiledMethod *method = (STCompiledMethod *)detagObject(assoc->value);
			STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
			char *start = (char *)bytecodes->array;
			char *end = start + getValue(bytecodes->header.size);
			if(start <= caller && end >= caller )
				return assoc->value;
			CObjectPointer mthd = findInstructionSequenceThatIncludesIPInMethodLiterals( caller, method->literals);
			if( mthd != generalError )
				return mthd;
		}
	}
	return generalError;
}

extern CObjectPointer Trait;
extern CObjectPointer ClassTrait;
CObjectPointer findMethodThatIncludesIPInTraitComposition( char *caller, STTraitComposition *traitComposition) {
	int i;
	STArray *transformations = (STArray *)detagObject(traitComposition->transformations);
	for(i = 0; i < getSize(traitComposition->transformations); i++) {
		STClass *itemClass = (STClass *)detagObject(getClass(transformations->array[i]));
		STObject *item = detagObject(transformations->array[i]);
		if( item == detagObject(nil) )
			continue;
		if( itemClass == (STClass *)detagObject(Trait) || itemClass == (STClass *)detagObject(ClassTrait) ) {
			STTrait *trait = (STTrait *)item;
			CObjectPointer method = findMethodThatIncludesIPInMethodDictionary( caller, (STDictionary *)detagObject(trait->methodDict) );
			if( method != generalError )
				return method;
			if(trait->traitComposition != nil ) {
				method = findMethodThatIncludesIPInTraitComposition( caller, (STTraitComposition *)detagObject(trait->traitComposition) );
				if( method != generalError )
					return method;
			}
		}
	}
	return generalError;
}

extern CObjectPointer Test;
CObjectPointer findMethodThatIncludesIPInClass( char *caller, STClass *cls ) {
	CObjectPointer method = findMethodThatIncludesIPInMethodDictionary(caller, (STDictionary *)detagObject(cls->methods) );
	if( method != generalError )
		return method;
	if(cls->traitComposition != nil ) {
		method = findMethodThatIncludesIPInTraitComposition(caller, (STTraitComposition *)detagObject(cls->traitComposition) );
		if( method != generalError )
			return method;
	}
	return generalError;
}

extern CObjectPointer Class;
extern CObjectPointer Metaclass;
extern CObjectPointer Dictionary;
CObjectPointer findMethodThatIncludesIPForObjectInClassHierarchy( char *caller, CObjectPointer obj ) {
	/* Find the method in the class hierarchy of the object only */
	CObjectPointer class = getClass(obj);
	STClass *cls = (STClass *)detagObject(class);
	if( getClass(getClass(class)) != Metaclass)
		return generalError;
	while( cls != (STClass *)detagObject(nil) ) {
		CObjectPointer method = findMethodThatIncludesIPInClass(caller, cls );
		if( method != generalError )
			return method;
		cls = (STClass *)detagObject(cls->superClass);
	};
	return generalError;
}

extern CObjectPointer Smalltalk;
CObjectPointer findMethodThatIncludesIP( char *caller ){
	/* Find the method in every class */
	STSystemDictionary *st = (STSystemDictionary *)detagObject(Smalltalk);
	STArray *classArray = (STArray *)detagObject(st->array);
	int i;
	CObjectPointer class;
	for(i = 0; i < getValue(classArray->header.size)>>2 ; i++ ) {
		STAssociation *assoc = (STAssociation *)detagObject(classArray->array[i]);
		if( assoc != (STAssociation *)detagObject(nil) ) {
			class = assoc->value;
			if(getClass(getClass(class))==Metaclass) {
				CObjectPointer method = findMethodThatIncludesIPInClass(caller, (STClass *)detagObject(class) );
				if( method != generalError )
					return method;
				method = findMethodThatIncludesIPInClass(caller, (STClass *)detagObject(getClass(class)) );
				if( method != generalError )
					return method;
			}
		}
	}
	return generalError;
}

CObjectPointer findMethodThatIncludesIPForObject( char *caller, CObjectPointer obj ) {
	CObjectPointer method = findMethodThatIncludesIPForObjectInClassHierarchy(caller, obj ); //Faster
	if( method != generalError )
		return method;
	return findMethodThatIncludesIP( caller ); //Slower
}

typedef struct{
	STHeader header;
	CObjectPointer framePointer;
	CObjectPointer method;
	STInteger *pc;
} STMethodContext;

typedef struct{
	STHeader header;
	CObjectPointer framePointer;
	CObjectPointer method;
	STInteger *pc;
	CObjectPointer blockClosure;
} STBlockContext;

extern CObjectPointer FramePointer;
extern CObjectPointer MethodContext;
extern CObjectPointer BlockContext;
CObjectPointer contextForFramePointer( unsigned int *bp ) {
	unsigned int *basePointer = bp;
	char *caller;
	CObjectPointer receiver;
	CObjectPointer method;
	CObjectPointer blockClosure;

	do{
		caller = (char *)*(basePointer+1);
		basePointer = (unsigned int *)*basePointer;
		if(!basePointer)
			return nil;
		receiver = (CObjectPointer)*(basePointer+2);
		if( getClass(receiver) == BlockClosure )
			blockClosure = receiver;
		method = findMethodThatIncludesIPForObject( caller, receiver );
	} while( method == generalError);

	CObjectPointer framePointer = newVariableByteObject( FramePointer, newInteger(4) );
	STArray *fP = ( STArray * )detagObject( framePointer );
	fP->array[0] = (int)basePointer;
	if( getClass(method) == InstructionSequence ) {
		CObjectPointer newContext = instantiateClass(BlockContext);
		STBlockContext *context = (STBlockContext *)detagObject( newContext );
		context->framePointer = framePointer;
		context->method = method;
		STArray *bytecodes = (STArray *)detagObject(((STCompiledMethod *)detagObject(method))->bytecodes);
		int pc = caller - (char *) bytecodes->array;
		if( (pc < 0) || pc > getSize(tagObject((STObject *)bytecodes)) ){
			error("Can not have this pc");
		}
		context->pc = newInteger(pc);
		context->blockClosure = blockClosure;
		return newContext;
	}
	if( getClass(method) == CompiledMethod ) {
		CObjectPointer newContext = instantiateClass(MethodContext);
		STMethodContext *context = (STMethodContext *)detagObject( newContext );
		context->framePointer = framePointer;
		context->method = method;
		STArray *bytecodes = (STArray *)detagObject(((STCompiledMethod *)detagObject(method))->bytecodes);
		int pc = caller - (char *) bytecodes->array;
//		print(tagObject((STObject *)bytecodes));
		if( (pc < 0) || pc > getSize(tagObject((STObject *)bytecodes)) )
			error("Can not have this pc");
		context->pc = newInteger(pc);
		return newContext;
	}
	return generalError;
}

// primitive  209
CObjectPointer primitiveFramePointerThisContext( CObjectPointer framePointer ) {
	unsigned int *basePointer;	
	asm(	"movl %%ebp,%[basePointer]"
	: [basePointer] "=r" (basePointer) );
	basePointer = (unsigned int *)*basePointer;
	return contextForFramePointer( basePointer );
}

// primitive  210
CObjectPointer primitiveFramePointerAt( CObjectPointer frmPtr, STInteger *index ) {
	if(getClass(frmPtr) != FramePointer)
		return generalError;
	if(!isSmallInteger(index))
		return generalError;
	STArray *framePointer = (STArray *)detagObject( frmPtr );
	unsigned int *basePointer = (unsigned int *)framePointer->array[0];
	if(getValue(index)==1) // Return address
		return generalError;
	if(getValue(index)==0) //Get previous Context
		return contextForFramePointer( basePointer );
	return (CObjectPointer) *(basePointer+getValue(index));
}

typedef struct{
	STHeader header;
	CObjectPointer mainLoopProcess;
	CObjectPointer gtkLibrary;
}STGtk;

// primitive 211
extern void primDoGtkMainIteration();
CObjectPointer gtkMinLoopProcess;
CObjectPointer primitiveGtkStartMainLoop( CObjectPointer gtk ) {
	gtkMinLoopProcess = ((STGtk *)detagObject(gtk))->mainLoopProcess;
	if( gtkMinLoopProcess == nil )
//		error("Aqui");
		return generalError;
	if( getClass(gtkMinLoopProcess) != Process )
		return generalError;
	STProcess *process = (STProcess *)detagObject(gtkMinLoopProcess);
	process->stackPointer = newExternalAddress( &gtk );  // tricky, used to get base pointer
	process->state = symbol_suspended;
	primDoGtkMainIteration();
	process->state = symbol_running;
	process->stackPointer = nil;
	gtkMinLoopProcess = nil;
	return gtk;
}

extern CFunctionPointer _GCallback_Class_processCallbackC58_bytecodes;
extern CObjectPointer GCallback;
inline
CObjectPointer primExecuteGtkCallback( CObjectPointer handler ) {
	STProcess *process = (STProcess *)detagObject(gtkMinLoopProcess);
	CObjectPointer oldStackPointer = nil;
	// Callbacks can be nested, so we must signal the running state for the outer call only
	if(	process->state == symbol_suspended ) {
		oldStackPointer = process->stackPointer;
		process->stackPointer = nil;
		process->state = symbol_running;
	}
	CObjectPointer returnValue = _GCallback_Class_processCallbackC58_bytecodes( nil, handler );
	if(	oldStackPointer != nil ) {
		process->state = symbol_suspended;
		process->stackPointer = oldStackPointer;
	}
	return returnValue;
}

// primitive  212
CObjectPointer findSymbol( char* symbolName );
extern unsigned int objectMemorySize;
extern unsigned int maxObjectMemorySize;
extern unsigned int lowSpaceWatcherThreshold;
extern char *executableName;
extern CObjectPointer showMessageSends;
extern CObjectPointer lowSpaceSemaphore;
CObjectPointer primitiveKernelPropertyAt( CObjectPointer utilities, CObjectPointer property ) {
	if( property == findSymbol("ObjectMemorySize") )
		return (CObjectPointer)newIntegerOrLong(objectMemorySize);
	if( property == findSymbol("MaxObjectMemorySize") )
		return (CObjectPointer)newIntegerOrLong(maxObjectMemorySize);
	if( property == findSymbol("LowSpaceWatcherThreshold") )
		return (CObjectPointer)newIntegerOrLong( lowSpaceWatcherThreshold );
	if( property == findSymbol("ImageName") )
		return newString(imageName);
	if( property == findSymbol("ExecutableName") )
		return newString(executableName);
	if( property == findSymbol("ShowMessageSends") ) {
		if( showMessageSends == generalError )
			showMessageSends = false;
		return showMessageSends;
	}
	if( property == findSymbol("LowSpaceSemaphore") )
		return lowSpaceSemaphore;
	if( property == findSymbol("KernelDebugMode") ) {
#ifdef DEBUG
		return true;
#else
		return false;
#endif
	}
	print(property);
	error("primitiveKernelPropertyAt undefined property");
}

// primitive  213
CObjectPointer primitiveKernelPropertyAtPut( CObjectPointer utilities, CObjectPointer property, CObjectPointer value ) {
	if( property == findSymbol("MaxObjectMemorySize") && isSmallInteger( value ) ) {
		int size = getValue(value);
		if( size > MaxMemorySize )
			size = MaxMemorySize;
		maxObjectMemorySize = size;
		return (CObjectPointer)newInteger(size);
	}
	if( property == findSymbol("LowSpaceWatcherThreshold") && isSmallInteger( value ) ) {
		int size = getValue(value);
		if( size >= MaxMemorySize )
			size = MaxMemorySize - 1;
		lowSpaceWatcherThreshold = size;
		return (CObjectPointer)newInteger(size);
	}
	if( property == findSymbol("ShowMessageSends") && ( value == true ||  value == false ) )
		return showMessageSends = value;
	if( property == findSymbol("LowSpaceSemaphore") && getClass( value ) == findClass( "Semaphore" ) )
		return lowSpaceSemaphore = value;
	print(property);
	error("primitiveKernelPropertyAtPut undefined property");
}

int enterCriticalSection( void );
// primitive  214
CObjectPointer primitiveEnterCriticalSection( CObjectPointer rcvr ) {
	enterCriticalSection();
	return rcvr;
}

int leaveCriticalSection( void );
// primitive  215
CObjectPointer primitiveLeaveCriticalSection( CObjectPointer rcvr ) {
	leaveCriticalSection();
	return rcvr;
}

// primitive  216
CObjectPointer primitiveLibraryDataGetValue( CObjectPointer libraryData, CObjectPointer returnValue ) {
	STExternalSymbol *data = (STExternalSymbol *)detagObject( libraryData );
	STArray *handle = (STArray *)detagObject(data->handle);
	unsigned int *address = (unsigned int *)handle->array[0];
	STArray *value = (STArray *)detagObject( returnValue );
	value->array[0] = (CObjectPointer)*address;
	return libraryData;
}

// primitive  217
CObjectPointer primitiveLibraryDataSetValue( CObjectPointer libraryData, CObjectPointer argValue ) {
	STExternalSymbol *data = (STExternalSymbol *)detagObject( libraryData );
	STArray *handle = (STArray *)detagObject(data->handle);
	unsigned int *address = (unsigned int *)handle->array[0];
	STArray *value = (STArray *)detagObject( argValue );
	*address = (unsigned int)value->array[0];
	return libraryData;
}

// primitive  218
CObjectPointer primitiveBufferStoreIntoByteArray( CObjectPointer anExternalBuffer, CObjectPointer byteArray ) {
	STString *array = (STString *)detagObject( byteArray );
		if( array->header.info.type != VariableByteObject )
		return generalError;
	STExternalBuffer *buffer = (STExternalBuffer *)detagObject( anExternalBuffer );
	STArray *handle = (STArray *)detagObject(buffer->handle);
	char *source = (char *)handle->array[0];
	char *destination = array->string;
	int bufferSize = getValue(buffer->size);
	int arraySize = getValue(array->header.size);
	int sz = arraySize;
	if( sz > bufferSize ) {
		memset(destination+bufferSize, 0, arraySize-bufferSize);
		sz = bufferSize;
	}
	if( memcpy(destination, source, sz) != (char *)array->string )
		return generalError;
	return byteArray;
}
		
// primitive  249
CObjectPointer primitiveArrayBecomeOneWayCopyHash( void ) {
	error("primitiveArrayBecomeOneWayCopyHash");
}

CObjectPointer primitiveInvalid( CObjectPointer rcvr ) {
	return generalError;
}

__attribute__((regparm(1)))
CFunctionPointer _getPrimitiveIP( STInteger *primitiveNumber ) {
	switch(getValue(primitiveNumber)) {
		case 1: return (CFunctionPointer)&primitiveSmallIntegerAdd;
		case 2: return (CFunctionPointer)&primitiveSmallIntegerSubstract;
		case 3: return (CFunctionPointer)&primitiveSmallIntegerLess;
		case 4: return (CFunctionPointer)&primitiveSmallIntegerGreater;
		case 5: return (CFunctionPointer)&primitiveSmallIntegerLessOrEqual;
		case 6: return (CFunctionPointer)&primitiveSmallIntegerGreaterOrEqual;
		case 7: return (CFunctionPointer)&primitiveSmallIntegerEqual;
		case 8: return (CFunctionPointer)&primitiveSmallIntegerNotEqual;
		case 9: return (CFunctionPointer)&primitiveSmallIntegerMultiply;
		case 10: return (CFunctionPointer)&primitiveSmallIntegerDivide;
		case 11: return (CFunctionPointer)&primitiveSmallIntegerMod;
		case 12: return (CFunctionPointer)&primitiveSmallIntegerDiv;
		case 13: return (CFunctionPointer)&primitiveSmallIntegerQuo;
		case 14: return (CFunctionPointer)&primitiveSmallIntegerBitAnd;
		case 15: return (CFunctionPointer)&primitiveSmallIntegerBitOr;
		case 16: return (CFunctionPointer)&primitiveSmallIntegerBitXor;
		case 17: return (CFunctionPointer)&primitiveSmallIntegerBitShift;
		case 18: return (CFunctionPointer)&primitiveMakePoint;
		case 40: return (CFunctionPointer)&primitiveSmallIntegerAsFloat;
		case 41: return (CFunctionPointer)&primitiveFloatAdd;
		case 42: return (CFunctionPointer)&primitiveFloatSubstract;
		case 43: return (CFunctionPointer)&primitiveFloatLess;
		case 44: return (CFunctionPointer)&primitiveFloatGreater;
		case 45: return (CFunctionPointer)&primitiveFloatLessOrEqual;
		case 46: return (CFunctionPointer)&primitiveFloatGreaterOrEqual;
		case 47: return (CFunctionPointer)&primitiveFloatEqual;
		case 48: return (CFunctionPointer)&primitiveFloatNotEqual;
		case 49: return (CFunctionPointer)&primitiveFloatMultiply;
		case 50: return (CFunctionPointer)&primitiveFloatDivide;
		case 51: return (CFunctionPointer)&primitiveFloatTruncated;
		case 52: return (CFunctionPointer)&primitiveFloatFractionalPart;
		case 53: return (CFunctionPointer)&primitiveFloatExponent;
		case 54: return (CFunctionPointer)&primitiveFloatTimesTwoPower;
		case 55: return (CFunctionPointer)&primitiveFloatSquareRoot;
		case 56: return (CFunctionPointer)&primitiveFloatSine;
		case 57: return (CFunctionPointer)&primitiveFloatArcTan;
		case 58: return (CFunctionPointer)&primitiveFloatLogN;
		case 59: return (CFunctionPointer)&primitiveFloatExp;
		case 60: return (CFunctionPointer)&primitiveObjectAt;
		case 61: return (CFunctionPointer)&primitiveObjectAtPut;
		case 62: return (CFunctionPointer)&primitiveObjectSize;
		case 63: return (CFunctionPointer)&primitiveStringAt;
		case 64: return (CFunctionPointer)&primitiveStringAtPut;
		case 65: return (CFunctionPointer)&primitiveStreamNext;
		case 66: return (CFunctionPointer)&primitiveStreamNextPut;
		case 67: return (CFunctionPointer)&primitiveStreamAtEnd;
		case 70: return (CFunctionPointer)&primitiveBehaviorNew;
		case 71: return (CFunctionPointer)&primitiveBehaviorNewWithArg;
		case 72: return (CFunctionPointer)&primitiveArrayBecomeOneWay;
		case 73: return (CFunctionPointer)&primitiveInstVarAt;
		case 74: return (CFunctionPointer)&primitiveInstVarAtPut;
		case 75: return (CFunctionPointer)&primitiveProtoObjectIdentityHash;
		case 77: return (CFunctionPointer)&primitiveBehaviorSomeInstance;
		case 78: return (CFunctionPointer)&primitiveProtoObjectNextInstance;
		case 81: return (CFunctionPointer)&primitiveBlockClosureValue;
		case 83: return (CFunctionPointer)&primitiveObjectPerform;
		case 84: return (CFunctionPointer)&primitiveObjectPerformWithArgs;
		case 85: return (CFunctionPointer)&primitiveSemaphoreSignal;
		case 86: return (CFunctionPointer)&primitiveProcessWaitSemaphore;
		case 88: return (CFunctionPointer)&primitiveProcessRemoteSuspend;
		case 89: return (CFunctionPointer)&primitiveObjectBehaviorFlushCache;
		case 97: return (CFunctionPointer)&primitiveSnapshot;
		case 100: return (CFunctionPointer)&primitiveObjectPerformWithArgsInSuper;
		case 110: return (CFunctionPointer)&primitiveObjectEquivalent;
		case 111: return (CFunctionPointer)&primitiveObjectClass;
		case 112: return (CFunctionPointer)&primitiveBytesLeft;
		case 113: return (CFunctionPointer)&primitiveSystemDictionaryQuit;
		case 115: return (CFunctionPointer)&primitiveObjectChangeClass;
		case 116: return (CFunctionPointer)&primitiveCompiledMethodFlushCache;
		case 119: return (CFunctionPointer)&primitiveBehaviorFlushCacheSelective;
		case 123: return (CFunctionPointer)&primitiveObjectKernelDisplay;
		case 124: return (CFunctionPointer)&primitiveGetMoreCore;
		case 128: return (CFunctionPointer)&primitiveArrayBecome;
		case 130: return (CFunctionPointer)&primitiveGarbageCollect;
		case 131: return (CFunctionPointer)&primitiveGarbageCollectMost;
		case 132: return (CFunctionPointer)&primitiveObjectPointsTo;
		case 135: return (CFunctionPointer)&primitiveTimeMillisecondClock;
		case 136: return (CFunctionPointer)&primitiveSignalAtMilliseconds;
		case 137: return (CFunctionPointer)&primitiveTimeSecondsClock;
		case 145: return (CFunctionPointer)&primitiveByteArrayConstantFill;
		case 148: return (CFunctionPointer)&primitiveObjectClone;
		case 150: return (CFunctionPointer)&primitiveJumpBufferNew;
		case 151: return (CFunctionPointer)&primitiveJumpBufferReturn;
		case 153: return (CFunctionPointer)&primitiveBlockClosureOnDo;
		case 154: return (CFunctionPointer)&primitiveRelocationInfoAddress;
		case 155: return (CFunctionPointer)&primitiveFunctionRelocationAddressRelativeTo;
		case 167: return (CFunctionPointer)&primitiveProcessorSchedulerYield;
		case 188: return (CFunctionPointer)&primitiveObjectExecuteMethodWithArgs;
		case 190: return (CFunctionPointer)&primitiveExternalBufferByteAt;
		case 191: return (CFunctionPointer)&primitiveExternalBufferByteAtPut;
		case 192: return (CFunctionPointer)&primitiveSemaphoreCreate;
		case 193: return (CFunctionPointer)&primitiveSemaphoreDestroy;
		case 195: return (CFunctionPointer)&primitiveProcessSchedulerActiveProcess;
		case 196: return (CFunctionPointer)&primitiveBeInitialProcess;
		case 197: return (CFunctionPointer)&primitiveFunctionInvokeReceiverWithArguments;
		case 198: return (CFunctionPointer)&primitiveDynamicLibraryOpen;
		case 199: return (CFunctionPointer)&primitiveBufferLoadFromByteArray;
		case 200: return (CFunctionPointer)&primitiveProcessTerminate;
		case 201: return (CFunctionPointer)&primitiveExternalLibraryOpen;
		case 202: return (CFunctionPointer)&primitiveExternalLibraryClose;
		case 203: return (CFunctionPointer)&primitiveExternalLibraryFindFunction;
		case 204: return (CFunctionPointer)&primitiveExternalFunctionInvokeWithArguments;
		case 205: return (CFunctionPointer)&primitiveExternalAddressForArray;
		case 206: return (CFunctionPointer)&primitiveExternalAddressForObject;
		case 207: return (CFunctionPointer)&primitiveExternalAddressToString;
		case 208: return (CFunctionPointer)&primitiveProcessExecute;
		case 209: return (CFunctionPointer)&primitiveFramePointerThisContext;
		case 210: return (CFunctionPointer)&primitiveFramePointerAt;
		case 211: return (CFunctionPointer)&primitiveGtkStartMainLoop;
		case 212: return (CFunctionPointer)&primitiveKernelPropertyAt;
		case 213: return (CFunctionPointer)&primitiveKernelPropertyAtPut;
		case 214: return (CFunctionPointer)&primitiveEnterCriticalSection;
		case 215: return (CFunctionPointer)&primitiveLeaveCriticalSection;
		case 216: return (CFunctionPointer)&primitiveLibraryDataGetValue;
		case 217: return (CFunctionPointer)&primitiveLibraryDataSetValue;
		case 218: return (CFunctionPointer)&primitiveBufferStoreIntoByteArray;
		case 249: return (CFunctionPointer)&primitiveArrayBecomeOneWayCopyHash;
		default: return (CFunctionPointer)&primitiveInvalid;
	}
}
