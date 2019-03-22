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

// +
CObjectPointer primitiveFloatAdd( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C43(CObjectPointer rcvr, CObjectPointer arg) {
/*	if(areIntegers(rcvr,arg)) {
	error("special method + for SmallInteger called");
	int result;
	asm(	"movl $1,%%edx\n\t"
	"movl %[rcvr],%[result]\n\t"
	"addl %[arg],%[result]\n\t"
	"cmovol %%edx,%[result]"
	: [result] "=r" (result)
	: [rcvr] "r" (rcvr), [arg] "r" (arg)
	: "edx" );
	if ( result != generalError )
	return (CObjectPointer) result;
}
	*/	return primitiveFloatAdd( rcvr, arg );
}

// -
CObjectPointer primitiveFloatSubstract( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C45(CObjectPointer rcvr, CObjectPointer arg) {
/*	if(areIntegers(rcvr,arg)) {
	error("special method - for SmallInteger called");
	int result;
	asm(	"movl $1,%%edx\n\t"
	"movl %[rcvr],%[result]\n\t"
	"subl %[arg],%[result]\n\t"
	"cmovol %%edx,%[result]"
	: [result] "=r" (result)
	: [rcvr] "r" (rcvr), [arg] "r" (arg)
	: "edx" );
	if ( result != generalError )
	return (CObjectPointer) result;
}
	*/	return primitiveFloatSubstract( rcvr, arg );
}

// *
CObjectPointer primitiveFloatMultiply( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C42(CObjectPointer rcvr, CObjectPointer arg) {
	if(areIntegers(rcvr,arg)) {
		int receiver = getValue(rcvr);
		int argument = getValue(arg);
		int result = receiver * argument;
		if( ((argument == 0) || (result / argument == receiver)) && isIntegerValue(result) )
			return (CObjectPointer)newInteger( result );
	}
	return primitiveFloatMultiply( rcvr, arg );
}

// /
CObjectPointer primitiveSmallIntegerDivide( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer primitiveFloatDivide( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C47(CObjectPointer rcvr, CObjectPointer arg) {
	CObjectPointer result;
	if( ( result = primitiveSmallIntegerDivide( rcvr, arg ) ) != generalError )
		return result;
	return primitiveFloatDivide( rcvr, arg );
}

// //
CObjectPointer primitiveSmallIntegerDiv( CObjectPointer rcvr, CObjectPointer argu );
CObjectPointer C47C47( CObjectPointer rcvr, CObjectPointer arg ) {
	return primitiveSmallIntegerDiv( rcvr, arg );
}

// barra invertida doble
CObjectPointer primitiveSmallIntegerMod(CObjectPointer rcvr, CObjectPointer argu );
CObjectPointer C92C92( CObjectPointer rcvr, CObjectPointer arg ) {
	return primitiveSmallIntegerMod( rcvr, arg );
}

// bitShift:
CObjectPointer primitiveSmallIntegerBitShift(CObjectPointer rcvr, CObjectPointer arg);
CObjectPointer bitShiftC58(CObjectPointer rcvr, CObjectPointer arg) {
	return primitiveSmallIntegerBitShift( rcvr, arg );
}

// bitAnd:
CObjectPointer primitiveSmallIntegerBitAnd(CObjectPointer rcvr, CObjectPointer arg);
CObjectPointer bitAndC58(CObjectPointer rcvr, CObjectPointer arg) {
	return primitiveSmallIntegerBitAnd( rcvr, arg );
}

// bitOr:
CObjectPointer primitiveSmallIntegerBitOr(CObjectPointer rcvr, CObjectPointer arg);
CObjectPointer bitOrC58(CObjectPointer rcvr, CObjectPointer arg) {
	return primitiveSmallIntegerBitOr( rcvr, arg );
}

// value
extern CObjectPointer BlockClosure;
CObjectPointer value( CObjectPointer rcvr ) {
	if( getClass( rcvr ) != BlockClosure )
		return generalError;
	STBlockClosure *block = (STBlockClosure *)detagObject(rcvr);
	STCompiledMethod *method = (STCompiledMethod *)detagObject(block->method);
	STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
	CFunctionPointer functionPointer = (CFunctionPointer)bytecodes->array;
	return functionPointer( block->environment );
}

// value: 
CObjectPointer valueC58( CObjectPointer rcvr, CObjectPointer value ) {
	if( getClass( rcvr ) != BlockClosure )
		return generalError;
	STBlockClosure *block = (STBlockClosure *)detagObject(rcvr);
	STCompiledMethod *method = (STCompiledMethod *)detagObject(block->method);
	STArray *bytecodes = (STArray *)detagObject(method->bytecodes);
	CFunctionPointer functionPointer = (CFunctionPointer)bytecodes->array;
	return functionPointer( block->environment, value );
}

// @
CObjectPointer primitiveMakePoint(CObjectPointer rcvr, CObjectPointer arg);
CObjectPointer C64(CObjectPointer rcvr, CObjectPointer arg) {
	return primitiveMakePoint( rcvr, arg );
}

// ==
CObjectPointer C61C61(CObjectPointer rcvr, CObjectPointer arg) {
	return getTrueOrFalse( rcvr == arg );
}

// ~=
CObjectPointer primitiveFloatNotEqual( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C126C61(CObjectPointer rcvr, CObjectPointer arg) {
	// Tricky!, we dont need to detag in order to compare 
	if(areIntegers(rcvr,arg)) {
		error("special method ~= for SmallInteger called");
		return getTrueOrFalse( ((int)rcvr) != ((int)arg) );
	}
	return primitiveFloatNotEqual( rcvr, arg );
}

// =
CObjectPointer primitiveFloatEqual( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C61(CObjectPointer rcvr, CObjectPointer arg) {
	// Tricky!, we dont need to detag in order to compare 
	if(areIntegers(rcvr,arg)) {
		error("special method = for SmallInteger called");
		return getTrueOrFalse( ((int)rcvr) == ((int)arg) );
	}
	return primitiveFloatEqual( rcvr, arg );
}

// <=
CObjectPointer primitiveFloatLessOrEqual( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C60C61(CObjectPointer rcvr, CObjectPointer arg) {
	// Tricky!, we dont need to detag in order to compare 
	if(areIntegers(rcvr,arg)) {
		error("special method <= for SmallInteger called");
		return getTrueOrFalse( ((int)rcvr) <= ((int)arg) );
	}
	return primitiveFloatLessOrEqual( rcvr, arg );
}

// >
CObjectPointer primitiveFloatGreater( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C62(CObjectPointer rcvr, CObjectPointer arg) {
	// Tricky!, we dont need to detag in order to compare 
	if(areIntegers(rcvr,arg)) {
		error("special method > for SmallInteger called");
		return getTrueOrFalse( ((int)rcvr) > ((int)arg) );
	}
	return primitiveFloatGreater( rcvr, arg );
}

// >=
CObjectPointer primitiveFloatGreaterOrEqual( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C62C61(CObjectPointer rcvr, CObjectPointer arg) {
	// Tricky!, we dont need to detag in order to compare 
	if(areIntegers(rcvr,arg)) {
		error("special method >= for SmallInteger called");
		return getTrueOrFalse( ((int)rcvr) >= ((int)arg) );
	}
	return primitiveFloatGreaterOrEqual( rcvr, arg );
}

// <
CObjectPointer primitiveFloatLess( CObjectPointer rcvr, CObjectPointer arg );
CObjectPointer C60(CObjectPointer rcvr, CObjectPointer arg) {
	// Tricky!, we dont need to detag in order to compare 
	if(areIntegers(rcvr,arg)) {
		error("special method < for SmallInteger called");
		return getTrueOrFalse( ((int)rcvr) < ((int)arg) );
	}
	return primitiveFloatLess( rcvr, arg );
}

CObjectPointer class( CObjectPointer rcvr) {
	return getClass(rcvr);
}
