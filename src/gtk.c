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
#include <gtk/gtk.h>

typedef struct  {
	STHeader header;
	CObjectPointer handler;
	CObjectPointer performer;
	CObjectPointer type;
	CObjectPointer arguments;
}STGCallback;

CObjectPointer primExecuteGtkCallback( CObjectPointer handler );
gboolean signalCallback(GObject* object, gpointer data, gint paramValuesSize, const GValue* paramValues) {
	int i;
	STGCallback *callback = (STGCallback *)data;
	callback->arguments = newObject( Array, newInteger(paramValuesSize + 1) );
	STArray *arguments = (STArray *)detagObject( callback->arguments );
	arguments->array[ 0 ] = newExternalAddress( object );
	for (i = 0; i <= (paramValuesSize - 1); i ++ )
		arguments->array[ i + 1 ] = newExternalAddress( (void *)&paramValues[ i ] );
	gdk_threads_leave();
	CObjectPointer returnObject = primExecuteGtkCallback( callback->handler );
	gdk_threads_enter();
	if(returnObject  == true)
		return TRUE;
	if(returnObject  == false)
		return FALSE;
	error("Unexpected return value in callback");
}
				 
void signalCallbackMarshal (GClosure     *closure,
							GValue       *return_value,
	   guint         n_param_values,
	const GValue *param_values,
 gpointer      invocation_hint,
 gpointer      marshal_data)
{
	
	typedef gboolean (*SignalCallbackFunc) (GObject *object, gpointer data, gint paramValuesSize, const GValue *paramValues);
	register SignalCallbackFunc callback;
	register GCClosure *cc = (GCClosure*) closure;
	register gpointer data1, data2;
	gboolean retVal;

	if (G_CCLOSURE_SWAP_DATA (closure))
	{
		data1 = closure->data;
		data2 = g_value_peek_pointer (param_values + 0);
	}
	else
	{
		data1 = g_value_peek_pointer (param_values + 0);
		data2 = closure->data;
	}

	callback = (SignalCallbackFunc) (marshal_data ? marshal_data : cc->callback);
	retVal = callback (data1, data2, (gint) (n_param_values - 1), param_values + 1);
	if (return_value)
		g_value_set_boolean (return_value, retVal);
}

void sqGWeakNotifyObject(gpointer data, GObject* object){
	STGCallback *callback = (STGCallback *)data;
	callback->arguments = newObject( Array, newInteger( 1 ) );
	STArray *arguments = (STArray *)detagObject( callback->arguments );
	arguments->array[ 0 ] = newExternalAddress( object );
	gdk_threads_leave();
	CObjectPointer returnObject = primExecuteGtkCallback( callback->handler );
	gdk_threads_enter();
	if(returnObject != true)
		error("return from callback incomplete");
}

int primitiveGEnumValueGetValue( GEnumValue *structOop ) {
	return structOop->value;
}

gchar *primitiveGEnumValueGetValueNick( GEnumValue *structOop ) {
	return (gchar *)structOop->value_nick;
}

int primitiveGFlagsValueGetValue( GFlagsValue *structOop ) {
	return structOop->value;
}

char *primitiveGFlagsValueGetValueNick( GFlagsValue *structOop ) {
	return (char *)structOop->value_nick;
}

char *primitiveGtkStockItemGetStockId( GtkStockItem *structOop ) {
	return structOop->stock_id;
}

char *primitiveGtkStockItemGetLabel( GtkStockItem *structOop ) {
	return structOop->label;
}

int primitiveGtkStockItemGetKeyval( GtkStockItem *structOop ) {
	return structOop->keyval;
}

int primitiveGtkStockItemGetModifier( GtkStockItem *structOop ) {
	return structOop->modifier;
}

int primitiveGFlagsGetNValues( GFlagsClass *structOop ) {
	return structOop->n_values;
}

int primitiveGValueType( GValue *gValueOop ) {
	return G_VALUE_TYPE(gValueOop);
}

int primitiveGParamSpecValueType( GParamSpec *gParamSpecOop ) {
	return G_PARAM_SPEC_VALUE_TYPE(gParamSpecOop);
}

int primitiveGTypeFundamental( int gType ) {
	return G_TYPE_FUNDAMENTAL(gType);
}

int primitiveGTypeIsFundamental( int gType ) {
	return G_TYPE_IS_FUNDAMENTAL(gType);
}

int primitiveGTypeBoolean( void ) {
	return G_TYPE_BOOLEAN;
}

int primitiveGTypeObject( void ) {
	return G_TYPE_OBJECT;
}

int primitiveGTypeBoxed( void ) {
	return G_TYPE_BOXED;
}

int primitiveGTypeInt( void ) {
	return G_TYPE_INT;
}

int primitiveGTypeUint( void ) {
	return G_TYPE_UINT;
}

int primitiveGTypeInterface( void ) {
	return G_TYPE_INTERFACE;
}

int primitiveGTypeString( void ) {
	return G_TYPE_STRING;
}

int primitiveGTypeLong( void ) {
	return G_TYPE_LONG;
}

int primitiveGTypeUlong( void ) {
	return G_TYPE_ULONG;
}

int primitiveGTypeEnum( void ) {
	return G_TYPE_ENUM;
}

int primitiveGTypeFloat( void ) {
	return G_TYPE_FLOAT;
}

int primitiveGTypeParam( void ) {
	return G_TYPE_PARAM;
}

int primitiveGTypePointer( void ) {
	return G_TYPE_POINTER;
}

int primitiveGdkTypePixbuf( void ) {
	return GDK_TYPE_PIXBUF;
}

int primitiveGObjectType( GObject *gObjectOop ) {
	return G_TYPE_FROM_INSTANCE(gObjectOop);
}

char *primitiveGValueTypeName( GTypeInstance *gValueOop ) {
	return (char *)G_VALUE_TYPE_NAME( gValueOop );
}

gchar *primitiveGObjectTypeName( GObject *gObjectOop ) {
	return (gchar *)G_OBJECT_TYPE_NAME(gObjectOop);
}

GtkTreeIter *primitiveGtkTreeIterNew(void) {
	return g_new0(GtkTreeIter, 1);
}

GtkTextIter *primitiveGtkTextIterNew(void) {
	return g_new0(GtkTextIter, 1);
}

GValue *primitiveGValueNew(void) {
	return g_new0(GValue, 1);
}

GtkStockItem *primitiveGtkStockItemNew(void) {
	return g_new0(GtkStockItem, 1);
}

inline
void primDoGtkMainIteration( void ) {
	gtk_main();
}

GtkWidget *primitiveGtkDialogGetVBox( GtkDialog *dialog ) {
	return dialog->vbox;
}

GtkWidget *primitiveGtkDialogGetActionArea( GtkDialog *dialog ) {
	return dialog->action_area;
}
