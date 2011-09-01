/*
 * Copyright (C) 2001-2011 Ideaworks3D Ltd.
 * All Rights Reserved.
 *
 * This document is protected by copyright, and contains information
 * proprietary to Ideaworks Labs.
 * This file consists of source code released by Ideaworks Labs under
 * the terms of the accompanying End User License Agreement (EULA).
 * Please do not use this program/source code before you have read the
 * EULA and have agreed to be bound by its terms.
 */
/*
Internal header for the s3eFacebook extension.

This file should be used for any common function definitions etc that need to
be shared between the platform-dependent and platform-indepdendent parts of
this extension.
*/

/*
 * NOTE: This file was origianlly written by the extension builder, but will not
 * be overwritten (unless --force is specified) and is intended to be modified.
 */


#ifndef S3EFACEBOOK_H_INTERNAL
#define S3EFACEBOOK_H_INTERNAL

#include "s3eFacebook.h"
#include "s3eFacebook_autodefs.h"


/**
 * Initialise the extension.  This is called once then the extension is first
 * accessed by s3eregister.  If this function returns S3E_RESULT_ERROR the
 * extension will be reported as not-existing on the device.
 */
s3eResult s3eFacebookInit();

/**
 * Platform-specific initialisation, implemented on each platform
 */
s3eResult s3eFacebookInit_platform();

/**
 * Terminate the extension.  This is called once on shutdown, but only if the
 * extension was loader and Init() was successful.
 */
void s3eFacebookTerminate();

/**
 * Platform-specific termination, implemented on each platform
 */
void s3eFacebookTerminate_platform();
s3eFBSession* s3eFBInit_platform(const char* appId);

s3eResult s3eFBTerminate_platform(s3eFBSession* session);

s3eResult s3eFBSession_Login_platform(s3eFBSession* session, s3eFBLoginCallbackFn cb, void* cbData, const char** pPermissions, int numPermissions);

s3eResult s3eFBSession_Logout_platform(s3eFBSession* session);

s3eBool s3eFBSession_LoggedIn_platform(s3eFBSession* session);

const char* s3eFBSession_AccessToken_platform(s3eFBSession* session);

s3eFBDialog* s3eFBDialog_WithAction_platform(s3eFBSession* session, const char* action);

s3eResult s3eFBDialog_Delete_platform(s3eFBDialog* dialog);

s3eResult s3eFBDialog_AddParamString_platform(s3eFBDialog* dialog, const char* name, const char* value);

s3eResult s3eFBDialog_AddParamNumber_platform(s3eFBDialog* dialog, const char* name, int64 value);

s3eResult s3eFBDialog_Show_platform(s3eFBDialog* dialog, s3eFBDialogCallbackFn cb, void* cbData);

s3eBool s3eFBDialog_Error_platform(s3eFBDialog* dialog);

uint32 s3eFBDialog_ErrorCode_platform(s3eFBDialog* dialog);

const char* s3eFBDialog_ErrorString_platform(s3eFBDialog* dialog);

s3eBool s3eFBDialog_Complete_platform(s3eFBDialog* dialog);

const char* s3eFBDialog_DidNotCompleteWithUrl_platform(s3eFBDialog* dialog);

s3eFBRequest* s3eFBRequest_WithMethodName_platform(s3eFBSession* session, const char* methodName, const char* httpMethod);

s3eFBRequest* s3eFBRequest_WithGraphPath_platform(s3eFBSession* session, const char* graphPath, const char* httpMethod);

s3eFBRequest* s3eFBRequest_WithURL_platform(s3eFBSession* session, const char* url, const char* httpMethod);

s3eResult s3eFBRequest_Delete_platform(s3eFBRequest* request);

s3eResult s3eFBRequest_AddParamString_platform(s3eFBRequest* request, const char* name, const char* value);

s3eResult s3eFBRequest_AddParamNumber_platform(s3eFBRequest* request, const char* name, int64 value);

s3eResult s3eFBRequest_Send_platform(s3eFBRequest* request, s3eFBRequestCallbackFn cb, void* cbData);

s3eBool s3eFBRequest_Error_platform(s3eFBRequest* request);

uint32 s3eFBRequest_ErrorCode_platform(s3eFBRequest* request);

const char* s3eFBRequest_ErrorString_platform(s3eFBRequest* request);

s3eBool s3eFBRequest_Complete_platform(s3eFBRequest* request);

s3eFBResponseType s3eFBRequest_ResponseType_platform(s3eFBRequest* request);

const char* s3eFBRequest_ResponseRaw_platform(s3eFBRequest* request);

const char* s3eFBRequest_ResponseAsString_platform(s3eFBRequest* request);

int64 s3eFBRequest_ResponseAsNumber_platform(s3eFBRequest* request);

int s3eFBRequest_ResponseArrayCount_platform(s3eFBRequest* request);

const char* s3eFBRequest_ResponseArrayItemAsString_platform(s3eFBRequest* request, int index);

s3eBool s3eFBRequest_ResponseDictionaryContainsItem_platform(s3eFBRequest* request, const char* key);

const char* s3eFBRequest_ResponseDictionaryItemAsString_platform(s3eFBRequest* request, const char* key);


#endif /* S3EFACEBOOK_H_INTERNAL */
