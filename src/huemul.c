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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "huemul.h"
#include <stdio.h>
#include <stdlib.h>

void *mainStackTop;
char *imageName;
char *executableName;
//void setKeypress(void);
//void resetKeypress(void);
//void setSystemMaxMem( void );
CObjectPointer initializeObjectMemory( void );
void initializeSignals( void );
CObjectPointer loadImage(char *imageName);
CObjectPointer runHuemul( int argc, char *argv[] );
extern CObjectPointer Smalltalk;
int main(int argc, char *argv[]) {
	printf("Huemul Smalltalk - Version 0.8\n");
	if (argc != 2) {
		printf("\nUsage:\n\t%s file.image\n", argv[0]);
		return (1);
	}
	mainStackTop = &argc;  //tricky
	printf("Initializing...\n");
	executableName = argv[0];
	imageName = argv[1];
//	setSystemMaxMem();
	initializeObjectMemory();
	initializeSignals();
	initializeMethodCache();
//	setKeypress();	
	if( loadImage(argv[1]) != generalError ) {
		printf("done\n");
		runHuemul( argc, argv );
	}
//	resetKeypress();
	return EXIT_SUCCESS;
}
