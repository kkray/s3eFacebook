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
Generic implementation of the s3eFacebook extension.
This file should perform any platform-indepedentent functionality
(e.g. error checking) before calling platform-dependent implementations.
*/

/*
 * NOTE: This file was origianlly written by the extension builder, but will not
 * be overwritten (unless --force is specified) and is intended to be modified.
 */


#include "s3eFacebook_internal.h"

#define S3E_CURRENT_EXT FACEBOOK
#include "s3eEdkError.h"
#include "IwDebug.h"

s3eResult s3eFacebookInit()
{
    //Add any generic initialisation code here
    return s3eFacebookInit_platform();
}

void s3eFacebookTerminate()
{
    //Add any generic termination code here
    s3eFacebookTerminate_platform();
}

s3eFBSession* s3eFBInit(const char* appId)
{
    if (!appId)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBInit_platform(appId);
}

s3eResult s3eFBTerminate(s3eFBSession* session)
{
    if (!session)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBTerminate_platform(session);
}

s3eResult s3eFBSession_Login(s3eFBSession* session, s3eFBLoginCallbackFn cb, void* cbData, const char** pPermissions, int numPermissions)
{
    if (!session)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBSession_Login_platform(session, cb, cbData, pPermissions, numPermissions);
}

s3eResult s3eFBSession_Logout(s3eFBSession* session)
{
    if (!session)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBSession_Logout_platform(session);
}

s3eBool s3eFBSession_LoggedIn(s3eFBSession* session)
{
    if (!session)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_FALSE;
    }
    
    return s3eFBSession_LoggedIn_platform(session);
}

const char* s3eFBSession_AccessToken(s3eFBSession* session)
{
    if (!session)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBSession_AccessToken_platform(session);
}

s3eFBDialog* s3eFBDialog_WithAction(s3eFBSession* session, const char* action)
{
    if (!session || !action)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBDialog_WithAction_platform(session, action);
}

s3eResult s3eFBDialog_Delete(s3eFBDialog* dialog)
{
    if (!dialog)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBDialog_Delete_platform(dialog);
}

s3eResult s3eFBDialog_AddParamString(s3eFBDialog* dialog, const char* name, const char* value)
{
    if (!dialog || !name || !value)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }

    return s3eFBDialog_AddParamString_platform(dialog, name, value);
}

s3eResult s3eFBDialog_AddParamNumber(s3eFBDialog* dialog, const char* name, int64 value)
{
    if (!dialog || !name)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBDialog_AddParamNumber_platform(dialog, name, value);
}

s3eResult s3eFBDialog_Show(s3eFBDialog* dialog, s3eFBDialogCallbackFn cb, void* cbData)
{
    if (!dialog)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBDialog_Show_platform(dialog, cb, cbData);
}

s3eBool s3eFBDialog_Error(s3eFBDialog* dialog)
{
    if (!dialog)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_FALSE;
    }

    return s3eFBDialog_Error_platform(dialog);
}

uint32 s3eFBDialog_ErrorCode(s3eFBDialog* dialog)
{
    if (!dialog)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return 0;
    }
    
    return s3eFBDialog_ErrorCode_platform(dialog);
}

const char* s3eFBDialog_ErrorString(s3eFBDialog* dialog)
{
    if (!dialog)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBDialog_ErrorString_platform(dialog);
}

s3eBool s3eFBDialog_Complete(s3eFBDialog* dialog)
{
    if (!dialog)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_FALSE;
    }
    
    return s3eFBDialog_Complete_platform(dialog);
}

const char* s3eFBDialog_DidNotCompleteWithUrl(s3eFBDialog* dialog)
{
    if (!dialog)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBDialog_DidNotCompleteWithUrl_platform(dialog);
}

s3eFBRequest* s3eFBRequest_WithMethodName(s3eFBSession* session, const char* methodName, const char* httpMethod)
{
    if (!session || !methodName)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }

    return s3eFBRequest_WithMethodName_platform(session, methodName, httpMethod);
}

s3eFBRequest* s3eFBRequest_WithGraphPath(s3eFBSession* session, const char* graphPath, const char* httpMethod)
{
    if (!session || !graphPath)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBRequest_WithGraphPath_platform(session, graphPath, httpMethod);
}

s3eFBRequest* s3eFBRequest_WithURL(s3eFBSession* session, const char* url, const char* httpMethod)
{
    if (!session || !url)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBRequest_WithURL_platform(session, url, httpMethod);
}

s3eResult s3eFBRequest_Delete(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBRequest_Delete_platform(request);
}

s3eResult s3eFBRequest_AddParamString(s3eFBRequest* request, const char* name, const char* value)
{
    if (!request || !name || !value)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBRequest_AddParamString_platform(request, name, value);
}

s3eResult s3eFBRequest_AddParamNumber(s3eFBRequest* request, const char* name, int64 value)
{
    if (!request || !name)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBRequest_AddParamNumber_platform(request, name, value);
}

s3eResult s3eFBRequest_Send(s3eFBRequest* request, s3eFBRequestCallbackFn cb, void* cbData)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_RESULT_ERROR;
    }
    
    return s3eFBRequest_Send_platform(request, cb, cbData);
}

s3eBool s3eFBRequest_Error(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_FALSE;
    }
    
    return s3eFBRequest_Error_platform(request);
}

uint32 s3eFBRequest_ErrorCode(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return 0;
    }
    
    return s3eFBRequest_ErrorCode_platform(request);
}

const char* s3eFBRequest_ErrorString(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBRequest_ErrorString_platform(request);
}

s3eBool s3eFBRequest_Complete(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_FALSE;
    }
    
    return s3eFBRequest_Complete_platform(request);
}

s3eFBResponseType s3eFBRequest_ResponseType(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return UNKNOWN;
    }
    
    return s3eFBRequest_ResponseType_platform(request);
}

const char* s3eFBRequest_ResponseRaw(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }

    return s3eFBRequest_ResponseRaw_platform(request);
}

const char* s3eFBRequest_ResponseAsString(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }

    return s3eFBRequest_ResponseAsString_platform(request);
}

int64 s3eFBRequest_ResponseAsNumber(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return 0;
    }

    return s3eFBRequest_ResponseAsNumber_platform(request);
}

int s3eFBRequest_ResponseArrayCount(s3eFBRequest* request)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return 0;
    }

    return s3eFBRequest_ResponseArrayCount_platform(request);
}

const char* s3eFBRequest_ResponseArrayItemAsString(s3eFBRequest* request, int index)
{
    if (!request)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }

    return s3eFBRequest_ResponseArrayItemAsString_platform(request, index);
}

s3eBool s3eFBRequest_ResponseDictionaryContainsItem(s3eFBRequest* request, const char* key)
{
    if (!request || !key)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return S3E_FALSE;
    }

    return s3eFBRequest_ResponseDictionaryContainsItem_platform(request, key);
}

const char* s3eFBRequest_ResponseDictionaryItemAsString(s3eFBRequest* request, const char* key)
{
    if (!request || !key)
    {
        S3E_EXT_ERROR_SIMPLE(PARAM);
        return NULL;
    }
    
    return s3eFBRequest_ResponseDictionaryItemAsString_platform(request, key);
}