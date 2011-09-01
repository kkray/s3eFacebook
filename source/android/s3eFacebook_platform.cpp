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
android-specific implementation of the s3eFacebook extension.
Add any platform-specific functionality here.
*/

/*
 * NOTE: This file was origianlly written by the extension builder, but will not
 * be overwritten (unless --force is specified) and is intended to be modified.
 */


#include "s3eFacebook_internal.h"
#include "s3eEdk.h"
#include "s3eEdk_android.h"
#include <jni.h>
#include "IwDebug.h"

enum s3eFacebookCallback
{
    S3E_FACEBOOK_LOGIN,
    S3E_FACEBOOK_DIALOG,
    S3E_FACEBOOK_REQUEST,
    S3E_FACEBOOK_CALLBACK_MAX
};

static void s3eFacebook_LoginCallback(JNIEnv *env, jobject _this, jobject session, bool loginResult);
static void s3eFacebook_DialogCallback(JNIEnv *env, jobject _this, jobject dialog, bool dialogResult);
static void s3eFacebook_RequestCallback(JNIEnv *env, jobject _this, jobject request, bool requestResult);

static jobject g_Obj;
static jmethodID g_s3eFBInit;
static jmethodID g_s3eFBTerminate;
static jmethodID g_s3eFBSession_Login;
static jmethodID g_s3eFBSession_Logout;
static jmethodID g_s3eFBSession_LoggedIn;
static jmethodID g_s3eFBSession_AccessToken;
static jmethodID g_s3eFBDialog_WithAction;
static jmethodID g_s3eFBDialog_Delete;
static jmethodID g_s3eFBDialog_AddParamString;
static jmethodID g_s3eFBDialog_AddParamNumber;
static jmethodID g_s3eFBDialog_Show;
static jmethodID g_s3eFBDialog_Error;
static jmethodID g_s3eFBDialog_ErrorCode;
static jmethodID g_s3eFBDialog_ErrorString;
static jmethodID g_s3eFBDialog_Complete;
static jmethodID g_s3eFBDialog_DidNotCompleteWithUrl;
static jmethodID g_s3eFBRequest_WithMethodName;
static jmethodID g_s3eFBRequest_WithGraphPath;
static jmethodID g_s3eFBRequest_WithURL;
static jmethodID g_s3eFBRequest_Delete;
static jmethodID g_s3eFBRequest_AddParamString;
static jmethodID g_s3eFBRequest_AddParamNumber;
static jmethodID g_s3eFBRequest_Send;
static jmethodID g_s3eFBRequest_Error;
static jmethodID g_s3eFBRequest_ErrorCode;
static jmethodID g_s3eFBRequest_ErrorString;
static jmethodID g_s3eFBRequest_Complete;
static jmethodID g_s3eFBRequest_ResponseType;
static jmethodID g_s3eFBRequest_ResponseRaw;
static jmethodID g_s3eFBRequest_ResponseAsString;
static jmethodID g_s3eFBRequest_ResponseAsNumber;
static jmethodID g_s3eFBRequest_ResponseArrayCount;
static jmethodID g_s3eFBRequest_ResponseArrayItemAsString;
static jmethodID g_s3eFBRequest_ResponseDictionaryContainsItem;
static jmethodID g_s3eFBRequest_ResponseDictionaryItemAsString;

static jfieldID g_s3eFBRequest_LoginIndex;
static jfieldID g_s3eFBRequest_DialogIndex;
static jfieldID g_s3eFBRequest_RequestIndex;

static char* g_RetStr;
static int g_RetStrLen = 16384;

static jobject g_KnownObjects[64] = {0};

struct JavaString
{
    jstring _Ref;

    JavaString ( const char *Buffer )
    {
        JNIEnv *env = s3eEdkJNIGetEnv();
        _Ref = env->NewStringUTF ( Buffer );
    }

    JavaString ( jstring Ref )
    {
        _Ref = Ref;
    }

    ~JavaString()
    {
        JNIEnv *env = s3eEdkJNIGetEnv();
        env->DeleteLocalRef ( _Ref );
    }

    operator jstring()
    {
        return _Ref;
    }
};

//Java object handles are not unique, so we need to be able to
//get a unique handle to each object for the purposes of callbacks
jobject getKnownObject(jobject obj)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    IwTrace(FACEBOOK_VERBOSE, ("Getting known object %p", obj));
    for (uint i=0;i<sizeof(g_KnownObjects)/sizeof(g_KnownObjects[0]);i++)
    {
        if (g_KnownObjects[i])
            IwTrace(FACEBOOK_VERBOSE, ("Comparing with %p", g_KnownObjects[i]));
        if (env->IsSameObject(obj, g_KnownObjects[i]))
            return g_KnownObjects[i];
    }

    return NULL;
}

void addKnownObject(jobject obj)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    IwTrace(FACEBOOK_VERBOSE, ("Adding known object %p", obj));
    if (getKnownObject(obj))
        return;

    for (uint i=0;i<sizeof(g_KnownObjects)/sizeof(g_KnownObjects[0]);i++)
    {
        if (!g_KnownObjects[i])
        {
            g_KnownObjects[i] = obj;
            return;
        }
    }

    IwAssertMsg(FACEBOOK, false, ("Out of known object storage!"));
}

void removeKnownObejct(jobject obj)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    for (uint i=0;i<sizeof(g_KnownObjects)/sizeof(g_KnownObjects[0]);i++)
    {
        if (env->IsSameObject(obj, g_KnownObjects[i]))
        {
            g_KnownObjects[i] = NULL;
        }
    }
}

const char* getCString(jstring str)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    if (!str)
        return NULL;
    jboolean free;
    const char* res = env->GetStringUTFChars(str, &free);
    g_RetStrLen = strlen(res);
    s3eEdkReallocOS(g_RetStr, g_RetStrLen);
    strncpy(g_RetStr, res, g_RetStrLen);
    env->ReleaseStringUTFChars(str, res);
    return g_RetStr;
}

s3eResult s3eFacebookInit_platform()
{
    //Alloc buffer for returning strings
    g_RetStr = (char*)s3eEdkMallocOS(g_RetStrLen);

    //Get a pointer to the JVM using the JNI extension
    JavaVM* jvm = (JavaVM*)s3eEdkJNIGetVM();

    if (!jvm)
        return S3E_RESULT_ERROR;

    //Get the environment from the pointer
    JNIEnv* env = s3eEdkJNIGetEnv();

    //Get the extension class
    jclass cls = env->FindClass("s3eFacebook");

    //Get its constructor
    jmethodID cons = env->GetMethodID(cls, "<init>", "()V");

    //Construct the java class
    g_Obj = env->NewObject(cls, cons);

    //Get all the extension methods
    g_s3eFBInit = env->GetMethodID(cls, "s3eFBInit", "(Ljava/lang/String;)Ljava/lang/Object;");
    if (!g_s3eFBInit)
        goto end;

    g_s3eFBTerminate = env->GetMethodID(cls, "s3eFBTerminate", "(Ljava/lang/Object;)I");
    if (!g_s3eFBTerminate)
        goto end;

    g_s3eFBSession_Login = env->GetMethodID(cls, "s3eFBSession_Login", "(Ljava/lang/Object;[Ljava/lang/String;)I");
    if (!g_s3eFBSession_Login)
        goto end;

    g_s3eFBSession_Logout = env->GetMethodID(cls, "s3eFBSession_Logout", "(Ljava/lang/Object;)I");
    if (!g_s3eFBSession_Logout)
        goto end;

    g_s3eFBSession_LoggedIn = env->GetMethodID(cls, "s3eFBSession_LoggedIn", "(Ljava/lang/Object;)Z");
    if (!g_s3eFBSession_LoggedIn)
        goto end;

     g_s3eFBSession_AccessToken = env->GetMethodID(cls, "s3eFBSession_AccessToken", "(Ljava/lang/Object;)Ljava/lang/String;");
    if (!g_s3eFBSession_AccessToken)
        goto end;

    g_s3eFBDialog_WithAction = env->GetMethodID(cls, "s3eFBDialog_WithAction", "(Ljava/lang/Object;Ljava/lang/String;)Ljava/lang/Object;");
    if (!g_s3eFBDialog_WithAction)
        goto end;

    g_s3eFBDialog_Delete = env->GetMethodID(cls, "s3eFBDialog_Delete", "(Ljava/lang/Object;)I");
    if (!g_s3eFBDialog_Delete)
        goto end;

    g_s3eFBDialog_AddParamString = env->GetMethodID(cls, "s3eFBDialog_AddParamString", "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;)I");
    if (!g_s3eFBDialog_AddParamString)
        goto end;

    g_s3eFBDialog_AddParamNumber = env->GetMethodID(cls, "s3eFBDialog_AddParamNumber", "(Ljava/lang/Object;Ljava/lang/String;J)I");
    if (!g_s3eFBDialog_AddParamNumber)
        goto end;

    g_s3eFBDialog_Show = env->GetMethodID(cls, "s3eFBDialog_Show", "(Ljava/lang/Object;)I");
    if (!g_s3eFBDialog_Show)
        goto end;

    g_s3eFBDialog_Error = env->GetMethodID(cls, "s3eFBDialog_Error", "(Ljava/lang/Object;)Z");
    if (!g_s3eFBDialog_Error)
        goto end;

    g_s3eFBDialog_ErrorCode = env->GetMethodID(cls, "s3eFBDialog_ErrorCode", "(Ljava/lang/Object;)I");
    if (!g_s3eFBDialog_ErrorCode)
        goto end;

    g_s3eFBDialog_ErrorString = env->GetMethodID(cls, "s3eFBDialog_ErrorString", "(Ljava/lang/Object;)Ljava/lang/String;");
    if (!g_s3eFBDialog_ErrorString)
        goto end;

    g_s3eFBDialog_Complete = env->GetMethodID(cls, "s3eFBDialog_Complete", "(Ljava/lang/Object;)Z");
    if (!g_s3eFBDialog_Complete)
        goto end;

    g_s3eFBDialog_DidNotCompleteWithUrl = env->GetMethodID(cls, "s3eFBDialog_DidNotCompleteWithUrl", "(Ljava/lang/Object;)Ljava/lang/String;");
    if (!g_s3eFBDialog_DidNotCompleteWithUrl)
        goto end;

    g_s3eFBRequest_WithMethodName = env->GetMethodID(cls, "s3eFBRequest_WithMethodName", "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
    if (!g_s3eFBRequest_WithMethodName)
        goto end;

    g_s3eFBRequest_WithGraphPath = env->GetMethodID(cls, "s3eFBRequest_WithGraphPath", "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
    if (!g_s3eFBRequest_WithGraphPath)
        goto end;

    g_s3eFBRequest_WithURL = env->GetMethodID(cls, "s3eFBRequest_WithURL", "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
    if (!g_s3eFBRequest_WithURL)
        goto end;

    g_s3eFBRequest_Delete = env->GetMethodID(cls, "s3eFBRequest_Delete", "(Ljava/lang/Object;)I");
    if (!g_s3eFBRequest_Delete)
        goto end;

    g_s3eFBRequest_AddParamString = env->GetMethodID(cls, "s3eFBRequest_AddParamString", "(Ljava/lang/Object;Ljava/lang/String;Ljava/lang/String;)I");
    if (!g_s3eFBRequest_AddParamString)
        goto end;

    g_s3eFBRequest_AddParamNumber = env->GetMethodID(cls, "s3eFBRequest_AddParamNumber", "(Ljava/lang/Object;Ljava/lang/String;J)I");
    if (!g_s3eFBRequest_AddParamNumber)
        goto end;

    g_s3eFBRequest_Send = env->GetMethodID(cls, "s3eFBRequest_Send", "(Ljava/lang/Object;)I");
    if (!g_s3eFBRequest_Send)
        goto end;

    g_s3eFBRequest_Error = env->GetMethodID(cls, "s3eFBRequest_Error", "(Ljava/lang/Object;)Z");
    if (!g_s3eFBRequest_Error)
        goto end;

    g_s3eFBRequest_ErrorCode = env->GetMethodID(cls, "s3eFBRequest_ErrorCode", "(Ljava/lang/Object;)I");
    if (!g_s3eFBRequest_ErrorCode)
        goto end;

    g_s3eFBRequest_ErrorString = env->GetMethodID(cls, "s3eFBRequest_ErrorString", "(Ljava/lang/Object;)Ljava/lang/String;");
    if (!g_s3eFBRequest_ErrorString)
        goto end;

    g_s3eFBRequest_Complete = env->GetMethodID(cls, "s3eFBRequest_Complete", "(Ljava/lang/Object;)Z");
    if (!g_s3eFBRequest_Complete)
        goto end;

    g_s3eFBRequest_ResponseType = env->GetMethodID(cls, "s3eFBRequest_ResponseType", "(Ljava/lang/Object;)I");
    if (!g_s3eFBRequest_ResponseType)
        goto end;

    g_s3eFBRequest_ResponseRaw = env->GetMethodID(cls, "s3eFBRequest_ResponseRaw", "(Ljava/lang/Object;)Ljava/lang/String;");
    if (!g_s3eFBRequest_ResponseRaw)
        goto end;

    g_s3eFBRequest_ResponseAsString = env->GetMethodID(cls, "s3eFBRequest_ResponseAsString", "(Ljava/lang/Object;)Ljava/lang/String;");
    if (!g_s3eFBRequest_ResponseAsString)
        goto end;

    g_s3eFBRequest_ResponseAsNumber = env->GetMethodID(cls, "s3eFBRequest_ResponseAsNumber", "(Ljava/lang/Object;)J");
    if (!g_s3eFBRequest_ResponseAsNumber)
        goto end;

    g_s3eFBRequest_ResponseArrayCount = env->GetMethodID(cls, "s3eFBRequest_ResponseArrayCount", "(Ljava/lang/Object;)I");
    if (!g_s3eFBRequest_ResponseArrayCount)
        goto end;

    g_s3eFBRequest_ResponseArrayItemAsString = env->GetMethodID(cls, "s3eFBRequest_ResponseArrayItemAsString", "(Ljava/lang/Object;I)Ljava/lang/String;");
    if (!g_s3eFBRequest_ResponseArrayItemAsString)
        goto end;

    g_s3eFBRequest_ResponseDictionaryContainsItem = env->GetMethodID(cls, "s3eFBRequest_ResponseDictionaryContainsItem", "(Ljava/lang/Object;Ljava/lang/String;)Z");
    if (!g_s3eFBRequest_ResponseDictionaryContainsItem)
        goto end;

    g_s3eFBRequest_ResponseDictionaryItemAsString = env->GetMethodID(cls, "s3eFBRequest_ResponseDictionaryItemAsString", "(Ljava/lang/Object;Ljava/lang/String;)Ljava/lang/String;");
    if (!g_s3eFBRequest_ResponseDictionaryItemAsString)
        goto end;

    end:

    const JNINativeMethod nativeMethodDefs[] =
    {
        {"LoginCallback",        "(Ljava/lang/Object;Z)V",        (void *)&s3eFacebook_LoginCallback},
        {"DialogCallback",        "(Ljava/lang/Object;Z)V",        (void *)&s3eFacebook_DialogCallback},
        {"RequestCallback",        "(Ljava/lang/Object;Z)V",        (void *)&s3eFacebook_RequestCallback},
    };

    env->RegisterNatives(cls, nativeMethodDefs, sizeof(nativeMethodDefs)/sizeof(nativeMethodDefs[0]));

    jthrowable exc = env->ExceptionOccurred();
    if (exc)
    {
        env->ExceptionDescribe();
        env->ExceptionClear();
        IwTrace(FACEBOOK, ("One or more java methods could not be found"));
        return S3E_RESULT_ERROR;
    }


    //Add any platform-specific initialisation code here
    return S3E_RESULT_SUCCESS;
}

void s3eFacebookTerminate_platform()
{
    s3eEdkFreeOS(g_RetStr);
    g_RetStr = NULL;
}

s3eFBSession* s3eFBInit_platform(const char* appId)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString appId_jstr(env->NewStringUTF(appId));
    jobject ret = env->CallObjectMethod(g_Obj, g_s3eFBInit, (jstring)appId_jstr);
    ret = env->NewGlobalRef(ret);
    addKnownObject(ret);
    return (s3eFBSession*)ret;
}

s3eResult s3eFBTerminate_platform(s3eFBSession* session)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    s3eResult res = (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBTerminate, (jobject)session);
    removeKnownObejct((jobject)session);
    env->DeleteGlobalRef((jobject)session);
    return res;
}

s3eResult s3eFBSession_Login_platform(s3eFBSession* session, s3eFBLoginCallbackFn cb, void* cbData, const char** pPermissions, int numPermissions)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    //register a one-shot callback
    s3eEdkCallbacksRegisterSpecific(
        S3E_EXT_FACEBOOK_HASH,
        S3E_FACEBOOK_CALLBACK_MAX,
        S3E_FACEBOOK_LOGIN,
        (s3eEdkCallbackSpecific)cb,
        cbData,
        S3E_FALSE,
        (void*)session);

    //create array of permissions
    jobjectArray permissions = env->NewObjectArray(numPermissions, env->FindClass("java/lang/String"), NULL);
    for (int i=0;i<numPermissions;i++)
    {
        env->SetObjectArrayElement(permissions, i, env->NewStringUTF(pPermissions[i]));
    }

    return (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBSession_Login, (jobject)session, permissions);
}

s3eResult s3eFBSession_Logout_platform(s3eFBSession* session)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBSession_Logout, (jobject)session);
}

s3eBool s3eFBSession_LoggedIn_platform(s3eFBSession* session)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (s3eBool)env->CallBooleanMethod(g_Obj, g_s3eFBSession_LoggedIn, (jobject)session);
}

const char* s3eFBSession_AccessToken_platform(s3eFBSession* session)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString str((jstring)env->CallObjectMethod(g_Obj, g_s3eFBSession_AccessToken, (jobject)session));
    return getCString((jstring)str);
}

s3eFBDialog* s3eFBDialog_WithAction_platform(s3eFBSession* session, const char* action)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString action_jstr(env->NewStringUTF(action));
    jobject ret = env->CallObjectMethod(g_Obj, g_s3eFBDialog_WithAction, (jobject)session, (jstring)action_jstr);
    ret = env->NewGlobalRef(ret);
    addKnownObject(ret);
    return (s3eFBDialog*)ret;
}

s3eResult s3eFBDialog_Delete_platform(s3eFBDialog* dialog)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    s3eResult res = (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBDialog_Delete, (jobject)dialog);
    removeKnownObejct((jobject)dialog);
    env->DeleteGlobalRef((jobject)dialog);
    return res;
}

s3eResult s3eFBDialog_AddParamString_platform(s3eFBDialog* dialog, const char* name, const char* value)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString name_jstr(env->NewStringUTF(name));
    JavaString value_jstr(env->NewStringUTF(value));
    return (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBDialog_AddParamString, (jobject)dialog, (jstring)name_jstr, (jstring)value_jstr);
}

s3eResult s3eFBDialog_AddParamNumber_platform(s3eFBDialog* dialog, const char* name, int64 value)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString name_jstr(env->NewStringUTF(name));
    return (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBDialog_AddParamNumber, (jobject)dialog, (jstring)name_jstr, value);
}

s3eResult s3eFBDialog_Show_platform(s3eFBDialog* dialog, s3eFBDialogCallbackFn cb, void* cbData)
{
    s3eEdkCallbacksRegisterSpecific(
        S3E_EXT_FACEBOOK_HASH,
        S3E_FACEBOOK_CALLBACK_MAX,
        S3E_FACEBOOK_DIALOG,
        (s3eEdkCallbackSpecific)cb,
        cbData,
        S3E_FALSE,
        (void*)dialog);

    JNIEnv* env = s3eEdkJNIGetEnv();
    return (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBDialog_Show, (jobject)dialog);
}

s3eBool s3eFBDialog_Error_platform(s3eFBDialog* dialog)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (s3eBool)env->CallBooleanMethod(g_Obj, g_s3eFBDialog_Error, (jobject)dialog);
}

uint32 s3eFBDialog_ErrorCode_platform(s3eFBDialog* dialog)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (uint32)env->CallIntMethod(g_Obj, g_s3eFBDialog_ErrorCode, (jobject)dialog);
}

const char* s3eFBDialog_ErrorString_platform(s3eFBDialog* dialog)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString str((jstring)env->CallObjectMethod(g_Obj, g_s3eFBDialog_ErrorString, (jobject)dialog));
    return getCString((jstring)str);
}

s3eBool s3eFBDialog_Complete_platform(s3eFBDialog* dialog)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (s3eBool)env->CallBooleanMethod(g_Obj, g_s3eFBDialog_Complete, (jobject)dialog);
}

const char* s3eFBDialog_DidNotCompleteWithUrl_platform(s3eFBDialog* dialog)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString str((jstring)env->CallObjectMethod(g_Obj, g_s3eFBDialog_DidNotCompleteWithUrl, (jobject)dialog));
    return getCString((jstring)str);
}

s3eFBRequest* s3eFBRequest_WithMethodName_platform(s3eFBSession* session, const char* methodName, const char* httpMethod)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString methodName_jstr(env->NewStringUTF(methodName));
    if (!httpMethod)
        httpMethod = "GET";
    JavaString httpMethod_jstr(env->NewStringUTF(httpMethod));
    jobject ret = env->CallObjectMethod(g_Obj, g_s3eFBRequest_WithMethodName, (jobject)session, (jstring)methodName_jstr, (jstring)httpMethod_jstr);
    ret = env->NewGlobalRef(ret);
    addKnownObject(ret);
    return (s3eFBRequest*)ret;
}

s3eFBRequest* s3eFBRequest_WithGraphPath_platform(s3eFBSession* session, const char* graphPath, const char* httpMethod)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString graphPath_jstr(env->NewStringUTF(graphPath));
    if (!httpMethod)
        httpMethod = "GET";
    JavaString httpMethod_jstr(env->NewStringUTF(httpMethod));
    jobject ret = env->CallObjectMethod(g_Obj, g_s3eFBRequest_WithGraphPath, (jobject)session, (jstring)graphPath_jstr, (jstring)httpMethod_jstr);
    ret = env->NewGlobalRef(ret);
    addKnownObject(ret);

    return (s3eFBRequest*)ret;
}

s3eFBRequest* s3eFBRequest_WithURL_platform(s3eFBSession* session, const char* url, const char* httpMethod)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString url_jstr(env->NewStringUTF(url));
    if (!httpMethod)
        httpMethod = "GET";
    JavaString httpMethod_jstr(env->NewStringUTF(httpMethod));
    jobject ret = env->CallObjectMethod(g_Obj, g_s3eFBRequest_WithURL, (jobject)session, (jstring)url_jstr, (jstring)httpMethod_jstr);
    ret = env->NewGlobalRef(ret);
    addKnownObject(ret);
    return (s3eFBRequest*)ret;
}

s3eResult s3eFBRequest_Delete_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    s3eResult res = (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBRequest_Delete, (jobject)request);
    removeKnownObejct((jobject)request);
    env->DeleteGlobalRef((jobject)request);
    return res;
}

s3eResult s3eFBRequest_AddParamString_platform(s3eFBRequest* request, const char* name, const char* value)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    IwTrace(FACEBOOK, ("s3eFBRequest_AddParamString %s %s",name, value));
    JavaString name_jstr(env->NewStringUTF(name));
    JavaString value_jstr(env->NewStringUTF(value));
    return (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBRequest_AddParamString, (jobject)request, (jstring)name_jstr, (jstring)value_jstr);
}

s3eResult s3eFBRequest_AddParamNumber_platform(s3eFBRequest* request, const char* name, int64 value)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString name_jstr(env->NewStringUTF(name));
    return (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBRequest_AddParamNumber, (jobject)request, (jstring)name_jstr, value);
}

s3eResult s3eFBRequest_Send_platform(s3eFBRequest* request, s3eFBRequestCallbackFn cb, void* cbData)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    s3eEdkCallbacksRegisterSpecific(
        S3E_EXT_FACEBOOK_HASH,
        S3E_FACEBOOK_CALLBACK_MAX,
        S3E_FACEBOOK_REQUEST,
        (s3eEdkCallbackSpecific)cb,
        cbData,
        S3E_FALSE,
        (void*)request);

    IwTrace(FACEBOOK, ("Calling send"));

    return (s3eResult)env->CallIntMethod(g_Obj, g_s3eFBRequest_Send, (jobject)request);
}

s3eBool s3eFBRequest_Error_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (s3eBool)env->CallBooleanMethod(g_Obj, g_s3eFBRequest_Error, (jobject)request);
}

uint32 s3eFBRequest_ErrorCode_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (uint32)env->CallIntMethod(g_Obj, g_s3eFBRequest_ErrorCode, (jobject)request);
}

const char* s3eFBRequest_ErrorString_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString res((jstring)env->CallObjectMethod(g_Obj, g_s3eFBRequest_ErrorString, (jobject)request));
    return getCString((jstring)res);
}

s3eBool s3eFBRequest_Complete_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (s3eBool)env->CallBooleanMethod(g_Obj, g_s3eFBRequest_Complete, (jobject)request);
}

s3eFBResponseType s3eFBRequest_ResponseType_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (s3eFBResponseType)env->CallIntMethod(g_Obj, g_s3eFBRequest_ResponseType, (jobject)request);
}

const char* s3eFBRequest_ResponseRaw_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString res((jstring)env->CallObjectMethod(g_Obj, g_s3eFBRequest_ResponseRaw, (jobject)request));
    return getCString((jstring)res);
}

const char* s3eFBRequest_ResponseAsString_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString res((jstring)env->CallObjectMethod(g_Obj, g_s3eFBRequest_ResponseAsString, (jobject)request));
    return getCString((jstring)res);
}

int64 s3eFBRequest_ResponseAsNumber_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return env->CallLongMethod(g_Obj, g_s3eFBRequest_ResponseAsNumber, (jobject)request);
}

int s3eFBRequest_ResponseArrayCount_platform(s3eFBRequest* request)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    return (int)env->CallIntMethod(g_Obj, g_s3eFBRequest_ResponseArrayCount, (jobject)request);
}

const char* s3eFBRequest_ResponseArrayItemAsString_platform(s3eFBRequest* request, int index)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString str((jstring)env->CallObjectMethod(g_Obj, g_s3eFBRequest_ResponseArrayItemAsString, (jobject)request, index));
    return getCString((jstring)str);
}

s3eBool s3eFBRequest_ResponseDictionaryContainsItem_platform(s3eFBRequest* request, const char* key)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString key_jstr(env->NewStringUTF(key));
    return (s3eBool)env->CallBooleanMethod(g_Obj, g_s3eFBRequest_ResponseDictionaryContainsItem, (jobject)request, (jstring)key_jstr);
}

const char* s3eFBRequest_ResponseDictionaryItemAsString_platform(s3eFBRequest* request, const char* key)
{
    JNIEnv* env = s3eEdkJNIGetEnv();
    JavaString key_jstr(env->NewStringUTF(key));
    JavaString str((jstring)env->CallObjectMethod(g_Obj, g_s3eFBRequest_ResponseDictionaryItemAsString, (jobject)request, (jstring)key_jstr));
    return getCString((jstring)str);
}

//Native functions called by java
void s3eFacebook_LoginCallback(JNIEnv *env, jobject _this, jobject session, bool loginResult)
{
    //loginResult is really s3eResult
    if (loginResult)
        IwTrace(FACEBOOK, ("Sending login success callback"));
    else
        IwTrace(FACEBOOK, ("Sending login failedcallback"));

    void* sessObject = (void*)getKnownObject(session);
    IwAssert(FACEBOOK, sessObject);

    s3eResult result = loginResult ? S3E_RESULT_SUCCESS: S3E_RESULT_ERROR;

    s3eEdkCallbacksEnqueue(S3E_EXT_FACEBOOK_HASH,
        S3E_FACEBOOK_LOGIN,
        &result ,
        4,
        sessObject,
        S3E_TRUE);
}

void s3eFacebook_DialogCallback(JNIEnv *env, jobject _this, jobject dialog, bool dialogResult)
{
    if (dialogResult)
        IwTrace(FACEBOOK, ("Sending dialog success callback"));
    else
        IwTrace(FACEBOOK, ("Sending dialog failed callback"));

    void* dlgObject = (void*)getKnownObject(dialog);
    IwAssert(FACEBOOK, dlgObject);

    s3eResult result = dialogResult ? S3E_RESULT_SUCCESS: S3E_RESULT_ERROR;

    //loginResult is really s3eResult
    s3eEdkCallbacksEnqueue(S3E_EXT_FACEBOOK_HASH,
        S3E_FACEBOOK_DIALOG,
        &result ,
        4,
        dlgObject,
        S3E_TRUE);
}

void s3eFacebook_RequestCallback(JNIEnv *env, jobject _this, jobject request, bool requestResult)
{
    if (requestResult)
        IwTrace(FACEBOOK, ("Sending request success callback"));
    else
        IwTrace(FACEBOOK, ("Sending request failed callback"));

    void* reqObject = (void*)getKnownObject(request);
    IwAssert(FACEBOOK, reqObject);

    s3eResult result = requestResult ? S3E_RESULT_SUCCESS: S3E_RESULT_ERROR;

    //loginResult is really s3eResult
    s3eEdkCallbacksEnqueue(S3E_EXT_FACEBOOK_HASH,
        S3E_FACEBOOK_REQUEST,
        &result,
        4,
        reqObject,
        S3E_TRUE);
}
