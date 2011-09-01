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
#define IW_USE_SYSTEM_STDLIB
#import "FBConnect.h"
#import "s3eFacebook.h"
#import "s3eFacebook_autodefs.h"

#include "IwList.h"
#include "IwDebug.h"
#include "s3eEdk.h"
#include "s3eEdk_iphone.h"

// Forward declarations
@class s3eFBSessionDelegate;
@class s3eFBDialogDelegate;
@class s3eFBRequestDelegate;

// Naughty... expose private function
@interface Facebook ()
- (void) openUrl:(NSString *)url params:(NSMutableDictionary *)params
    httpMethod:(NSString *)httpMethod delegate:(id<FBRequestDelegate>)delegate;
@end

// Helper functions
static char* CopyToCString(NSString* string)
{
    if (string)
    {
        const char* pStr = [string UTF8String];
        char* pCopy = (char*) s3eEdkMallocOS(strlen(pStr) + 1);
        if (pCopy)
        {
            strcpy(pCopy, pStr);
            return pCopy;
        }
    }
    return NULL;
}

static NSString* ToNSString(const char* pStr)
{
    if (pStr)
    {
        return [NSString stringWithUTF8String:pStr];
    }
    return nil;
}

s3eFBSession *g_s3eFBSession=NULL;

//----------------------------------------------------------------------------

// Helper Objective-C class to hold a response item and manage string memory
@interface ResponseItem : NSObject
{
    char* m_string;
}

@property(readonly) const char* string;

+(id) responseItemWithObject:(id)object;
@end

@implementation ResponseItem

@dynamic string;
-(const char*) string
{
    return m_string;
}

-(id) initWithObject:(id)object
{
    if (!(self = [super init]))
        return nil;

    m_string = CopyToCString([object description]);

    return self;
}

-(void) dealloc
{
    s3eEdkFreeOS(m_string);

    [super dealloc];
}

+(id) responseItemWithObject:(id)object
{
    return [[[[self class] alloc] initWithObject:object] autorelease];
}
@end


// Callback enums
//----------------------------------------------------------------------------
#define S3E_DEVICE_FACEBOOK S3E_EXT_FACEBOOK_HASH

typedef enum s3eFacebookCallback
{
    S3E_FACEBOOK_CALLBACK_LOGIN = 1,
    S3E_FACEBOOK_CALLBACK_DIALOG,
    S3E_FACEBOOK_CALLBACK_REQUEST,
    S3E_FACEBOOK_CALLBACK_MAX
} s3eFacebookCallback;

// Internal data structures..
//----------------------------------------------------------------------------

struct s3eFBSession
{
    s3eFBSession() : m_delegate(nil) {}

    s3eFBSessionDelegate *m_delegate;
};

struct s3eFBDialog
{
    s3eFBDialog() : m_delegate(nil) {}

    s3eFBDialogDelegate *m_delegate;
};

struct s3eFBRequest
{
    s3eFBRequest() : m_delegate(nil) {}

    s3eFBRequestDelegate *m_delegate;
};

// Delegate delcarations
//----------------------------------------------------------------------------

@interface s3eFBSessionDelegate : NSObject <FBSessionDelegate>
{
@public
    s3eFBSession m_s3eFBSession;
@private
    Facebook *m_session;
    NSString *m_appId;

    s3eBool m_loggedIn;
    s3eFBLoginCallbackFn m_loginCallback;
}

@property(readonly) Facebook* session;
@property(readonly) s3eBool loggedIn;

-(id) initWithSession:(Facebook *)session appId:(NSString *)appId;

-(void) loginWithCallback:(s3eFBLoginCallbackFn) cb cbData:(void *)cbData permissions:(NSArray *)permissions;
-(void) logout;
-(void) handleLoginResult:(s3eResult) result;
-(void) handleOpenURL:(NSURL *)URL;

- (void)fbDidLogin;
- (void)fbDidNotLogin:(BOOL)cancelled;
- (void)fbDidLogout;
@end

//----------------------------------------------------------------------------

@interface s3eFBDialogDelegate : NSObject <FBDialogDelegate>
{
@public
    s3eFBDialog m_s3eFBDialog;
@private
    Facebook *m_session;
    NSString *m_action;
    NSMutableDictionary *m_params;

    s3eBool m_complete;
    char *m_url;

    s3eBool m_error;
    uint32 m_errorCode;
    char *m_errorString;

    s3eFBDialogCallbackFn m_dialogCallback;
}
-(id) initWithSession:(Facebook*)session action:(NSString *)action;

- (void) showWithCallback:(s3eFBDialogCallbackFn) cb cbData:(void *)cbData;
- (void) addParamWithValue: (id)value forKey:(NSString*)key;
- (void) handleDialogResult:(s3eResult) result;

- (void)dialogDidComplete:(FBDialog *)dialog;
- (void)dialogDidNotCompleteWithUrl:(NSURL *)url;
- (void)dialogDidNotComplete:(FBDialog *)dialog;
- (void)dialog:(FBDialog*)dialog didFailWithError:(NSError *)error;

@property(readonly) s3eBool complete;
@property(readonly) const char* url;
@property(readonly) s3eBool error;
@property(readonly) uint32 errorCode;
@property(readonly) const char* errorString;
@end

//----------------------------------------------------------------------------

@interface s3eFBRequestDelegate : NSObject <FBRequestDelegate>
{
@public
    s3eFBRequest m_s3eFBRequest;
@private
    Facebook *m_session;
    NSString *m_methodName;
    NSString *m_graphPath;
    NSString *m_httpMethod;
    NSString *m_url;
    NSMutableDictionary *m_params;

    s3eBool m_complete;
    char* m_responseRaw;
    char* m_responseString;
    int64 m_responseNumber;
    NSMutableArray* m_responseArray;
    NSMutableDictionary* m_responseDictionary;
    s3eFBResponseType m_responseType;

    s3eBool m_error;
    uint32 m_errorCode;
    char *m_errorString;

    s3eFBRequestCallbackFn m_callback;
}

@property(readonly) s3eBool complete;
@property(readonly) const char* responseRaw;
@property(readonly) const char* responseString;
@property(readonly) int64 responseNumber;
@property(readonly) NSArray* responseArray;
@property(readonly) NSDictionary* responseDictionary;
@property(readonly) s3eFBResponseType responseType;
@property(readonly) s3eBool error;
@property(readonly) uint32 errorCode;
@property(readonly) const char* errorString;

-(id) initWithSession:(Facebook *)session methodName:(NSString *)methodName httpMethod:(NSString *)httpMethod;
-(id) initWithSession:(Facebook *)session graphPath:(NSString *)graphPath httpMethod:(NSString *)httpMethod;
-(id) initWithSession:(Facebook *)session url:(NSString *)url httpMethod:(NSString *)httpMethod;

- (void) sendWithCallback:(s3eFBRequestCallbackFn)cb cbData:(void *)cbData;
- (void) addParamWithValue: (id)value forKey:(NSString*)key;
- (void) handleRequestResult: (s3eResult) result;

- (void)requestLoading:(FBRequest*)request;
- (void)request:(FBRequest*)request didReceiveResponse:(NSURLResponse*)response;
- (void)request:(FBRequest*)request didFailWithError:(NSError*)error;
- (void)request:(FBRequest*)request didLoad:(id)result;
- (void)request:(FBRequest*)request didLoadRawResponse:(NSData*)data;
@end

// Delegate implementations
//----------------------------------------------------------------------------

@implementation s3eFBSessionDelegate

@synthesize session = m_session;
@synthesize loggedIn = m_loggedIn;

-(id) initWithSession:(Facebook *)session appId:(NSString *)appId
{
    if (!(self = [super init]))
        return nil;

    m_session = [session retain];
    m_appId = [appId retain];
    m_s3eFBSession.m_delegate = self;

    return self;
}

- (void)dealloc
{
    [m_session release];
    [m_appId release];

    [super dealloc];
}

-(void) loginWithCallback:(s3eFBLoginCallbackFn) cb cbData:(void *)cbData
    permissions:(NSArray *)permissions
{
    m_loginCallback = cb;
    if (m_loginCallback)
    {
         // Register callback
         EDK_CALLBACK_REG_SPECIFIC(
            FACEBOOK,
            LOGIN,
            (s3eEdkCallbackSpecific)cb,
            cbData,
            false,
            &m_s3eFBSession
         );
    }

    // Actually log in..
    [m_session authorize:permissions delegate:self];
}

-(void) logout
{
    [m_session logout:self];
}

-(void) handleLoginResult:(s3eResult) result
{
    //IwTrace(FACEBOOK_VERBOSE, ("Session is %s", [m_session isSessionValid] ? "valid" : "not valid"));

    m_loggedIn = (result == S3E_RESULT_SUCCESS) ? S3E_TRUE : S3E_FALSE;

    if (m_loginCallback)
    {
        // Callback
        s3eEdkCallbacksEnqueue(
            S3E_DEVICE_FACEBOOK,
            S3E_FACEBOOK_CALLBACK_LOGIN,
            &result,
            sizeof(result),
            &m_s3eFBSession,
            S3E_TRUE
        );
    }
}

- (void)handleOpenURL:(NSURL *)URL;
{
#ifdef IW_USE_TRACING
    const char *_url = [[URL absoluteString] UTF8String];
#endif
    IwTrace(FACEBOOK_VERBOSE, ("handleOpenURL Call with %s", _url));

    // Call FB controller method
    BOOL handled = [m_session handleOpenURL:URL];

    [URL release];
}

- (void)fbDidLogin
{
    IwTrace(FACEBOOK_VERBOSE,("didLogin"));
    [self handleLoginResult:S3E_RESULT_SUCCESS];
}

- (void)fbDidNotLogin:(BOOL)cancelled;
{
    IwTrace(FACEBOOK_VERBOSE,("sessionDidNotLogin "));
    [self handleLoginResult:S3E_RESULT_ERROR];
}

- (void)fbDidLogout
{
    IwTrace(FACEBOOK_VERBOSE,("sessionDidLogout"));
    m_loggedIn = S3E_FALSE;
}

@end

//----------------------------------------------------------------------------

@implementation s3eFBDialogDelegate

@synthesize complete = m_complete;
@dynamic url;
@synthesize error = m_error;
@synthesize errorCode = m_errorCode;
@dynamic errorString;

-(const char*) url
{
    return m_url;
}

-(const char*) errorString
{
    return m_errorString;
}

-(id) initWithSession:(Facebook*)session action:(NSString *)action;
{
    if (!(self = [super init]))
        return nil;

    m_session = [session retain];
    m_action = [action retain];
    m_params = [[NSMutableDictionary dictionaryWithCapacity: 0] retain];
    m_s3eFBDialog.m_delegate = self;

    return self;
}

- (void)dealloc
{
    [m_session release];
    [m_action release];
    [m_params release];

    s3eEdkFreeOS(m_url);
    s3eEdkFreeOS(m_errorString);

    [super dealloc];
}

- (void) showWithCallback:(s3eFBDialogCallbackFn) cb cbData:(void *)cbData
{
    m_dialogCallback = cb;
    if (m_dialogCallback)
    {
         // Register callback
         EDK_CALLBACK_REG_SPECIFIC(FACEBOOK, DIALOG, (s3eEdkCallbackSpecific)cb, cbData,
            false, &m_s3eFBDialog);
    }

    [m_session dialog:m_action andParams:m_params andDelegate:self];
}

- (void) addParamWithValue: (id)value forKey:(NSString*)key
{
    [m_params setValue:value forKey:key];
}

- (void) handleDialogResult:(s3eResult) result
{
    if (m_dialogCallback)
    {
        // Callback
        s3eEdkCallbacksEnqueue(
            S3E_DEVICE_FACEBOOK,
            S3E_FACEBOOK_CALLBACK_DIALOG,
            &result,
            sizeof(result),
            &m_s3eFBDialog,
            S3E_TRUE
        );
    }
}

- (void)dialogDidComplete:(FBDialog *)dialog
{
    IwTrace(FACEBOOK_VERBOSE,("dialogDidSucceed"));

    m_complete = S3E_TRUE;

    [self handleDialogResult:S3E_RESULT_SUCCESS];
}

- (void)dialogDidNotCompleteWithUrl:(NSURL *)url
{
    IwTrace(FACEBOOK_VERBOSE,("dialogDidNotCompleteWithUrl"));

    m_url = CopyToCString([url absoluteString]);

    [self handleDialogResult:S3E_RESULT_ERROR];
}

- (void)dialogDidNotComplete:(FBDialog *)dialog
{
    IwTrace(FACEBOOK_VERBOSE,("dialogDidCancel"));

    [self handleDialogResult:S3E_RESULT_ERROR];
}

- (void)dialog:(FBDialog*)dialog didFailWithError:(NSError *)error
{
    IwTrace(FACEBOOK_VERBOSE,("didFailWithError"));

    m_error = S3E_TRUE;
    m_errorCode = [error code];
    m_errorString = CopyToCString([error localizedDescription]);

    [self handleDialogResult:S3E_RESULT_ERROR];
}

@end

//----------------------------------------------------------------------------

@implementation s3eFBRequestDelegate

@synthesize complete = m_complete;
@dynamic responseRaw;
@dynamic responseString;
@synthesize responseNumber = m_responseNumber;
@dynamic responseArray;
@dynamic responseDictionary;
@synthesize responseType = m_responseType;
@synthesize error = m_error;
@synthesize errorCode = m_errorCode;
@dynamic errorString;

-(const char*) responseRaw
{
    return m_responseRaw;
}

-(const char*) responseString
{
    return m_responseString;
}

-(NSArray*) responseArray
{
    return m_responseArray;
}

-(NSDictionary*) responseDictionary
{
    return m_responseDictionary;
}

-(const char*) errorString
{
    return m_errorString;
}

-(id) initWithSession:(Facebook *)session
{
    if (!(self = [super init]))
        return nil;

    m_session = [session retain];
    m_params = [[NSMutableDictionary dictionaryWithCapacity: 0] retain];
    m_s3eFBRequest.m_delegate = self;

    return self;
}

-(id) initWithSession:(Facebook *)session
    methodName:(NSString *)methodName httpMethod:(NSString *)httpMethod
{
    if (!(self = [self initWithSession:session]))
        return nil;

    m_methodName = [methodName retain];
    m_httpMethod = [httpMethod retain];

    return self;
}

-(id) initWithSession:(Facebook *)session
    graphPath:(NSString *)graphPath httpMethod:(NSString *)httpMethod
{
    if (!(self = [self initWithSession:session]))
        return nil;

    m_graphPath = [graphPath retain];
    m_httpMethod = [httpMethod retain];

    return self;
}

-(id) initWithSession:(Facebook *)session
    url:(NSString *)url    httpMethod:(NSString *)httpMethod;
{
    if (!(self = [self initWithSession:session]))
        return nil;

    m_url = [url retain];
    m_httpMethod = [httpMethod retain];

    return self;
}

- (void)dealloc
{
    [m_session release];
    [m_methodName release];
    [m_graphPath release];
    [m_httpMethod release];
    [m_url release];
    [m_params release];

    s3eEdkFreeOS(m_responseRaw);
    s3eEdkFreeOS(m_responseString);
    [m_responseArray release];
    [m_responseDictionary release];
    s3eEdkFreeOS(m_errorString);

    [super dealloc];
}

- (void) sendWithCallback:(s3eFBRequestCallbackFn)cb cbData:(void *)cbData
{
    m_callback = cb;
    if (m_callback)
    {
        EDK_CALLBACK_REG_SPECIFIC(FACEBOOK, REQUEST, (s3eEdkCallbackSpecific)cb, cbData,
            false, &m_s3eFBRequest);
    }

    if (m_url)
    {
        NSString* httpMethod = m_httpMethod ? m_httpMethod : @"GET";

        IwTrace(FACEBOOK_VERBOSE, ("openUrl"));

        [m_session openUrl:m_url params:m_params
            httpMethod:httpMethod delegate:self];
    }
    else if (m_methodName)
    {
        NSString* httpMethod = m_httpMethod ? m_httpMethod : @"GET";

        IwTrace(FACEBOOK_VERBOSE, ("requestWithMethodName andParams andHttpMethod"));

        [m_session requestWithMethodName: m_methodName andParams: m_params
            andHttpMethod: httpMethod andDelegate: self];
    }
    else if (m_graphPath)
    {
        if ([m_params count] == 0)
        {
            IwTrace(FACEBOOK_VERBOSE, ("requestWithGraphPath"));

            [m_session requestWithGraphPath: m_graphPath andDelegate: self];
        }
        else if (!m_httpMethod)
        {
            IwTrace(FACEBOOK_VERBOSE, ("requestWithGraphPath andParams"));

            [m_session requestWithGraphPath: m_graphPath
                andParams: m_params andDelegate: self];
        }
        else
        {
            IwTrace(FACEBOOK_VERBOSE, ("requestWithGraphPath andParams andHttpMethod"));

            [m_session requestWithGraphPath: m_graphPath
                andParams: m_params andHttpMethod: m_httpMethod andDelegate: self];
        }
    }
    else
    {
        IwTrace(FACEBOOK_VERBOSE, ("requestWithParams"));

        [m_session requestWithParams: m_params andDelegate: self];
    }

    IwTrace(FACEBOOK_VERBOSE, ("Request sent"));
}

- (void) addParamWithValue: (id)value forKey:(NSString*)key
{
    [m_params setValue:value forKey:key];
}

- (void) handleRequestResult:(s3eResult) result
{
    if (m_callback)
    {
        // Callback
        s3eEdkCallbacksEnqueue(
            S3E_DEVICE_FACEBOOK,
            S3E_FACEBOOK_CALLBACK_REQUEST,
            &result,
            sizeof(result),
            &m_s3eFBRequest,
            S3E_TRUE
        );
    }
}

- (void)requestLoading:(FBRequest*)request
{
    IwTrace(FACEBOOK_VERBOSE,("requestLoading (with url %s)",
        [request.url UTF8String]));
}

- (void)request:(FBRequest*)request didReceiveResponse:(NSURLResponse*)response
{
    IwTrace(FACEBOOK_VERBOSE,("didReceiveResponse"));
}

- (void)request:(FBRequest*)request didFailWithError:(NSError*)error
{
    IwTrace(FACEBOOK_VERBOSE,("didFailWithError"));

    m_error = S3E_TRUE;
    m_errorCode = [error code];
    m_errorString = CopyToCString([error localizedDescription]);

    [self handleRequestResult:S3E_RESULT_ERROR];
}

- (void)request:(FBRequest*)request didLoad:(id)result
{
    IwTrace(FACEBOOK_VERBOSE,("didLoad"));

    // Called when a request returns and its response has been parsed into an object.
    // The resulting object may be a dictionary, an array, a string, or a number,
    // depending on thee format of the API response.

    if ([result isKindOfClass:[NSString class]])
    {
        m_responseType = STRING_TYPE;
        m_responseString = CopyToCString((NSString *)result);

        IwTrace(FACEBOOK_VERBOSE, ("Response string %s", m_responseString));
    }
    else if ([result isKindOfClass:[NSNumber class]])
    {
        m_responseType = NUMBER_TYPE;
        m_responseNumber = [((NSNumber *)result) longLongValue];

        IwTrace(FACEBOOK_VERBOSE, ("Response number %lld", m_responseNumber));
    }
    else if ([result isKindOfClass:[NSArray class]])
    {
        NSArray* array = (NSArray*)result;

        // Copy items in array into our own (converting as we go)
        m_responseType = ARRAY_TYPE;
        m_responseArray = [[NSMutableArray alloc] initWithCapacity:[array count]];

        NSEnumerator* enumerator = [array objectEnumerator];
        id anObject;

        while ((anObject = [enumerator nextObject]))
        {
            ResponseItem* reponseItem =
                [ResponseItem responseItemWithObject:anObject];

            // Array owns repsonse items
            [m_responseArray addObject:reponseItem];
        }

        IwTrace(FACEBOOK_VERBOSE, ("Response array %s", [[result description] UTF8String]));
    }
    else if ([result isKindOfClass:[NSDictionary class]])
    {
        NSDictionary* dictionary = (NSDictionary*)result;

        // Copy items in dictionary into our own (converting as we go)
        m_responseType = DICTIONARY_TYPE;
        m_responseDictionary = [[NSMutableDictionary alloc] initWithCapacity:[dictionary count]];

        NSEnumerator *enumerator = [dictionary keyEnumerator];
        id key;

        while ((key = [enumerator nextObject]))
        {
            ResponseItem* responseItem =
                [ResponseItem responseItemWithObject:[dictionary objectForKey:key]];

            // Dictionary owns repsonse items
            [m_responseDictionary setObject:responseItem forKey:key];
        }

        IwTrace(FACEBOOK_VERBOSE, ("Response dictionary %s", [[result description] UTF8String]));
    }
    else
    {
        m_responseType = UNKNOWN;

        IwTrace(FACEBOOK_VERBOSE, ("Response unknown"));
    }

    m_complete = S3E_TRUE;

    [self handleRequestResult:S3E_RESULT_SUCCESS];
}

- (void)request:(FBRequest*)request didLoadRawResponse:(NSData*)data
{
    IwTrace(FACEBOOK_VERBOSE,("didLoadRawResponse"));

    NSString* string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];

    m_responseRaw = CopyToCString(string);

    IwTrace(FACEBOOK_VERBOSE, ("Raw response %s", m_responseRaw));

    [string release];
}


@end

// Session init / terminate
//////////////////////////////////////////////////////////////////////////////

int32 _handleOpenURL(void* systemData, void* userData)
{
    IwTrace(FACEBOOK_VERBOSE,("_handleOpenURL CALLBACK %p, %p", systemData, userData));

    NSURL *url = (NSURL *)systemData;
    s3eFBSession *fbSession = (s3eFBSession *)userData;

    if (fbSession==g_s3eFBSession)
    {
        s3eFBSessionDelegate *_delegate = g_s3eFBSession->m_delegate;
        [_delegate handleOpenURL:url];
    }

    return 0;
}

s3eResult s3eFacebookInit_platform()
{
    return S3E_RESULT_SUCCESS;
}

void s3eFacebookTerminate_platform()
{
}

s3eFBSession* s3eFBInit_platform(const char *appId)
{
    NSString *_appId = ToNSString(appId);
    Facebook *_session = [[Facebook alloc] initWithAppId:_appId];

    if (_session == nil)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Failed New Session"));
        return NULL;
    }

    s3eFBSessionDelegate *_delegate =
        [[s3eFBSessionDelegate alloc] initWithSession:_session appId:_appId];

    // Delegate has retained session, so we can release our reference
    [_session release];

    IwTrace(FACEBOOK_VERBOSE, ("New Session: %x", (int)_session));

    s3eFBSession *s3eSession = &(_delegate->m_s3eFBSession);
    g_s3eFBSession = s3eSession;

    // Register callback
    s3eEdkCallbacksRegister(
        S3E_EDK_INTERNAL,
        S3E_EDK_CALLBACK_MAX,
        S3E_EDK_IPHONE_HANDLEOPENURL,
        _handleOpenURL,
        s3eSession,
        false
    );

    return s3eSession;
}

s3eResult s3eFBTerminate_platform(s3eFBSession *session)
{
    if (session)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Session terminated"));

        [session->m_delegate release];

        return S3E_RESULT_SUCCESS;
    }

    return S3E_RESULT_ERROR;
}



// Login / logout
//////////////////////////////////////////////////////////////////////////////

s3eResult s3eFBSession_Login_platform(s3eFBSession *session,
    s3eFBLoginCallbackFn cb, void *cbData,
    const char** pPermissions, int numPermissions)
{
    if (!session)
    {
        return S3E_RESULT_ERROR;
    }

    // Create permissions array
    NSArray* permissions;
    if (pPermissions && numPermissions >= 0)
    {
        NSMutableArray* array =
            [NSMutableArray arrayWithCapacity:numPermissions];
        for (int i=0; i<numPermissions; ++i)
        {
            [array addObject: ToNSString(pPermissions[i])];
        }

        permissions = array;
    }
    else
    {
        permissions = [NSArray arrayWithObjects:
            @"read_stream", @"publish_stream", @"offline_access", nil];
    }

    // Trace out if session expires??
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    if (defaults)
    {
        const char *expiration = "Never";
        NSDate* expirationDate = [defaults objectForKey:@"FBSessionExpires"];
        if (expirationDate) expiration = [[expirationDate description] UTF8String];

        IwTrace(FACEBOOK_VERBOSE, ("Session Expiration: %s", expiration));
    }

    IwTrace(FACEBOOK_VERBOSE, ("Login: %x %x", (int)cb, (int)cbData));

    // Currently the authorization flow does not return a user id anymore.
    // Need to make a request for current user id.
    [session->m_delegate loginWithCallback:cb cbData:cbData permissions:permissions];

    return S3E_RESULT_SUCCESS;
}

s3eResult s3eFBSession_Logout_platform(s3eFBSession* session)
{
    if (session)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Logout: %p", session));

        [session->m_delegate logout];

        return S3E_RESULT_SUCCESS;
    }

    return S3E_RESULT_ERROR;
}

s3eBool s3eFBSession_LoggedIn_platform(s3eFBSession* session)
{
    if (session)
    {
        return session->m_delegate.loggedIn;
    }
    return S3E_FALSE;
}

// Access Token
//////////////////////////////////////////////////////////////////////////////
const char* s3eFBSession_AccessToken_platform(s3eFBSession* session)
{
    if(session)
    {
        NSString* token = session->m_delegate.session.accessToken;
        return [token UTF8String];
    }
    return NULL;
}

// Dialogs
//////////////////////////////////////////////////////////////////////////////

s3eFBDialog* s3eFBDialog_WithAction_platform(s3eFBSession* session, const char* action)
{
    if (session && action)
    {
        NSString* _action = ToNSString(action);
        Facebook* _session = [session->m_delegate session];

        s3eFBDialogDelegate *_delegate =
            [[s3eFBDialogDelegate alloc] initWithSession:_session action:_action];

        s3eFBDialog *dialog = &_delegate->m_s3eFBDialog;

        IwTrace(FACEBOOK_VERBOSE, ("New dialog (WithAction): %p (%p)", dialog, session));

        return dialog;
    }

    return NULL;
}

s3eResult s3eFBDialog_Delete_platform(s3eFBDialog* dialog)
{
    if (dialog)
    {
        IwTrace(FACEBOOK_VERBOSE, ("s3eFBDialog_Delete"));

        [dialog->m_delegate release];

        return S3E_RESULT_SUCCESS;
    }

    return S3E_RESULT_ERROR;
}

s3eResult s3eFBDialog_AddParamString_platform(s3eFBDialog* dialog, const char* name, const char* value)
{
    if (dialog && name && value)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Add Param String"));

        NSString *_key = ToNSString(name);
        NSString *_value = ToNSString(value);

        [dialog->m_delegate addParamWithValue: _value forKey:_key];

        return S3E_RESULT_SUCCESS;
    }

    return S3E_RESULT_ERROR;
}

s3eResult s3eFBDialog_AddParamNumber_platform(s3eFBDialog* dialog, const char* name, int64 value)
{
    if (dialog && name)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Add Param Number"));

        NSString *_key = ToNSString(name);
        NSNumber *_value = [NSNumber numberWithInt:value];

        [dialog->m_delegate addParamWithValue:_value forKey:_key];

        return S3E_RESULT_SUCCESS;
    }

    return S3E_RESULT_ERROR;
}

s3eResult s3eFBDialog_Show_platform(s3eFBDialog* dialog, s3eFBDialogCallbackFn cb, void* cbData)
{
    if (dialog)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Showing dialog"));

        [dialog->m_delegate showWithCallback:cb cbData:cbData];

        return S3E_RESULT_SUCCESS;
    }

    return S3E_RESULT_ERROR;
}

s3eBool s3eFBDialog_Error_platform(s3eFBDialog* dialog)
{
    if (dialog)
        return dialog->m_delegate.error;
    return S3E_FALSE;
}

uint32 s3eFBDialog_ErrorCode_platform(s3eFBDialog* dialog)
{
    if (dialog)
        return dialog->m_delegate.errorCode;
    return 0;
}

const char* s3eFBDialog_ErrorString_platform(s3eFBDialog* dialog)
{
    if (dialog)
        return dialog->m_delegate.errorString;
    return NULL;
}

s3eBool s3eFBDialog_Complete_platform(s3eFBDialog* dialog)
{
    if (dialog)
        return dialog->m_delegate.complete;
    return S3E_FALSE;
}

const char* s3eFBDialog_DidNotCompleteWithUrl_platform(s3eFBDialog* dialog)
{
    if (dialog)
        return dialog->m_delegate.url;
    return NULL;
}

// Requests
//////////////////////////////////////////////////////////////////////////////

s3eFBRequest* s3eFBRequest_WithMethodName_platform(s3eFBSession* session,
    const char* methodName, const char* httpMethod)
{
    if (session && methodName)
    {
        NSString* _methodName = ToNSString(methodName);
        NSString* _httpMethod = ToNSString(httpMethod);
        Facebook* _session = [session->m_delegate session];

        s3eFBRequestDelegate *_delegate =
            [[s3eFBRequestDelegate alloc] initWithSession:_session
                methodName:_methodName httpMethod:_httpMethod];

        s3eFBRequest *request = &_delegate->m_s3eFBRequest;

        IwTrace(FACEBOOK_VERBOSE, ("New request (WithMethodName): %p (%p)", request, session));

        return request;
    }
    return NULL;
}

s3eFBRequest* s3eFBRequest_WithGraphPath_platform(s3eFBSession* session,
    const char* graphPath, const char* httpMethod)
{
    if (session && graphPath)
    {
        NSString* _graphPath = ToNSString(graphPath);
        NSString* _httpMethod = ToNSString(httpMethod);
        Facebook* _session = [session->m_delegate session];

        s3eFBRequestDelegate *_delegate =
            [[s3eFBRequestDelegate alloc] initWithSession:_session
                graphPath:_graphPath httpMethod:_httpMethod];

        s3eFBRequest *request = &_delegate->m_s3eFBRequest;

        IwTrace(FACEBOOK_VERBOSE, ("New request (WithGraphPath): %p (%p)", request, session));

        return request;
    }
    return NULL;
}

s3eFBRequest* s3eFBRequest_WithURL_platform(s3eFBSession* session,
    const char* url, const char* httpMethod)
{
    if (session && url)
    {
        NSString* _url = ToNSString(url);
        NSString* _httpMethod = ToNSString(httpMethod);
        Facebook* _session = [session->m_delegate session];

        s3eFBRequestDelegate *_delegate =
            [[s3eFBRequestDelegate alloc] initWithSession:_session
                url:_url httpMethod:_httpMethod];

        s3eFBRequest *request = &_delegate->m_s3eFBRequest;

        IwTrace(FACEBOOK_VERBOSE, ("New request (WithURL): %p (%p)", request, session));

        return request;
    }
    return NULL;
}

s3eResult s3eFBRequest_Delete_platform(s3eFBRequest* request)
{
    if (request)
    {
        IwTrace(FACEBOOK_VERBOSE, ("s3eFBRequest_Delete"));

        [request->m_delegate release];

        return S3E_RESULT_SUCCESS;
    }
    return S3E_RESULT_ERROR;
}

s3eResult s3eFBRequest_AddParamString_platform(s3eFBRequest *request, const char *name, const char *value)
{
    if (request && name && value)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Add Param String"));

        NSString *_key = ToNSString(name);
        NSString *_value = ToNSString(value);

        [request->m_delegate addParamWithValue: _value forKey:_key];

        return S3E_RESULT_SUCCESS;
    }
    return S3E_RESULT_ERROR;
}

s3eResult s3eFBRequest_AddParamNumber_platform(s3eFBRequest* request, const char* name, int64 value)
{
    if (request && name)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Add Param Number"));

        NSString *_key = ToNSString(name);
        NSNumber *_value = [NSNumber numberWithInt:value];

        [request->m_delegate addParamWithValue:_value forKey:_key];

        return S3E_RESULT_SUCCESS;
    }
    return S3E_RESULT_ERROR;
}

s3eResult s3eFBRequest_Send_platform(s3eFBRequest *request, s3eFBRequestCallbackFn cb, void *cbData)
{
    if (request)
    {
        IwTrace(FACEBOOK_VERBOSE, ("Sending request"));

        [request->m_delegate sendWithCallback:cb cbData:cbData];

        return S3E_RESULT_SUCCESS;
    }
    return S3E_RESULT_ERROR;
}

s3eBool s3eFBRequest_Error_platform(s3eFBRequest *request)
{
    if (request)
        return request->m_delegate.error;
    return S3E_FALSE;
}

uint32 s3eFBRequest_ErrorCode_platform(s3eFBRequest *request)
{
    if (request)
        return request->m_delegate.errorCode;
    return 0;
}

const char *s3eFBRequest_ErrorString_platform(s3eFBRequest *request)
{
    if (request)
        return request->m_delegate.errorString;
    return NULL;
}

s3eBool s3eFBRequest_Complete_platform(s3eFBRequest *request)
{
    if (request)
        return request->m_delegate.complete;
    return S3E_FALSE;
}

s3eFBResponseType s3eFBRequest_ResponseType_platform(s3eFBRequest *request)
{
    if (request)
        return request->m_delegate.responseType;
    return UNKNOWN;
}

const char* s3eFBRequest_ResponseRaw_platform(s3eFBRequest* request)
{
    if (request)
        return request->m_delegate.responseRaw;
    return NULL;
}

const char *s3eFBRequest_ResponseAsString_platform(s3eFBRequest *request)
{
    if (request)
        return request->m_delegate.responseString;
    return NULL;
}

int64 s3eFBRequest_ResponseAsNumber_platform(s3eFBRequest* request)
{
    if (request)
        return request->m_delegate.responseNumber;
    return 0;
}

int s3eFBRequest_ResponseArrayCount_platform(s3eFBRequest* request)
{
    if (request && request->m_delegate.responseArray)
        return [request->m_delegate.responseArray count];
    return 0;
}

const char* s3eFBRequest_ResponseArrayItemAsString_platform(s3eFBRequest* request, int index)
{
    if ((0 <= index) && (index < s3eFBRequest_ResponseArrayCount(request)))
    {
        ResponseItem* item = (ResponseItem*)
            [request->m_delegate.responseArray objectAtIndex:index];
        return [item string];
    }
    return NULL;
}

s3eBool s3eFBRequest_ResponseDictionaryContainsItem_platform(s3eFBRequest* request, const char* key)
{
    if (request && key && request->m_delegate.responseDictionary)
    {
        NSString* _key = ToNSString(key);

        if([request->m_delegate.responseDictionary objectForKey:_key] != nil)
        {
            return S3E_TRUE;
        }
    }
    return S3E_FALSE;
}

const char* s3eFBRequest_ResponseDictionaryItemAsString_platform(s3eFBRequest* request, const char* key)
{
    if (s3eFBRequest_ResponseDictionaryContainsItem(request, key))
    {
        ResponseItem* item = (ResponseItem*)
            [request->m_delegate.responseDictionary objectForKey:ToNSString(key)];

        return [item string];
    }
    return NULL;
}
