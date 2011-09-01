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
java implementation of the s3eFacebook extension.

Add android-specific functionality here.

These functions are called via JNI from native code.
*/
/*
 * NOTE: This file was origianlly written by the extension builder, but will not
 * be overwritten (unless --force is specified) and is intended to be modified.
 */

import android.util.Log;
import android.os.Bundle;
import android.os.Handler;

import org.json.JSONException;
import org.json.JSONObject;

import com.ideaworks3d.marmalade.LoaderActivity;
import com.ideaworks3d.marmalade.LoaderAPI;
import com.facebook.android.AsyncFacebookRunner;
import com.facebook.android.AsyncFacebookRunner.RequestListener;
import com.facebook.android.Facebook;
import com.facebook.android.FacebookError;
import com.facebook.android.DialogError;
import com.facebook.android.Facebook.DialogListener;
import com.facebook.android.Util;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.MalformedURLException;


class s3eFacebook
{

    final int S3E_RESULT_SUCCESS = 0;
    final int S3E_RESULT_ERROR = 1;

    final int STRING_TYPE = 0;
    final int NUMBER_TYPE = 1;
    final int ARRAY_TYPE = 2;
    final int DICTIONARY_TYPE= 3;
    final int UNKNOWN = 4;

    class s3eFBSession
    {
        private final class LoginDialogListener implements DialogListener {
            public void onComplete(Bundle values) {
                m_LoggedIn = true;
                Log.d("app", "login complete");
                LoginCallback((s3eFBSession)s3eFBSession.this, m_Facebook.isSessionValid());
            }

            public void onFacebookError(FacebookError error) {
                LoginCallback((s3eFBSession)s3eFBSession.this, false);
            }

            public void onError(DialogError error) {
                LoginCallback((s3eFBSession)s3eFBSession.this, false);
            }

            public void onCancel() {
                LoginCallback((s3eFBSession)s3eFBSession.this, false);
            }
        }

        String m_AppID;
        Facebook m_Facebook;
        boolean m_LoggedIn;

        s3eFBSession(String appId)
        {
            m_AppID = appId;
            m_Facebook = new Facebook(appId);
        }

        int Login(final String[] permissions)
        {
            LoaderActivity.m_Activity.runOnUiThread(new Runnable() {
                public void run() {
                    m_Facebook.authorize(LoaderActivity.m_Activity, permissions, Facebook.FORCE_DIALOG_AUTH, new LoginDialogListener());
                }
            });
            Log.d("app", "Done login");
            return 0;
        }

        boolean LoggedIn()
        {
            return m_Facebook.isSessionValid();
        }

        public int Logout()
        {
            try
            {
                m_Facebook.logout(LoaderActivity.m_Activity);
            }
            catch (Exception e)
            {

            }
            return S3E_RESULT_SUCCESS;
        }
    }


    class s3eFBDialog
    {
        private final class FBDialogListener implements DialogListener {
            public void onComplete(Bundle values) {
                final String postId = values.getString("post_id");
                m_Complete = true;
                DialogCallback(s3eFBDialog.this, postId != null);
            }

            public void onFacebookError(FacebookError error) {
                m_Error = true;
                DialogCallback(s3eFBDialog.this, false);
            }

            public void onError(DialogError error) {
                m_Error = true;
                m_DialogError = error;
                DialogCallback(s3eFBDialog.this, false);
            }

            public void onCancel() {
                m_Cancelled = true;
                DialogCallback(s3eFBDialog.this, false);
            }
        }



        String m_Action;
        Bundle m_Params;
        s3eFBSession m_Session;

        boolean m_Complete;
        boolean m_Error;
        boolean m_Cancelled;
        DialogError m_DialogError;


        s3eFBDialog(s3eFBSession session, String action)
        {
            m_Action = action;
            m_Params = new Bundle();
            m_Session = session;
        }

        void AddParamString(String param, String value)
        {
            m_Params.putString(param, value);
        }

        void AddParamLong(String param, long value)
        {
            m_Params.putLong(param, value);
        }

        int Show()
        {
            LoaderActivity.m_Activity.runOnUiThread(new Runnable() {
                public void run() {
                    m_Session.m_Facebook.dialog(LoaderActivity.m_Activity, m_Action, m_Params, new FBDialogListener());
                }});
            return S3E_RESULT_SUCCESS;
        }

        boolean GetComplete()
        {
            return m_Complete;
        }

        boolean GetError()
        {
            return m_Error;
        }

        boolean GetCancelled()
        {
            return m_Cancelled;
        }

        int GetErrorCode()
        {
            if (m_DialogError != null)
                return 1; //m_DialogError.getErrorCode(); getErrorCode is private. nice.
            else
                return 0;
        }

        String GetErrorString()
        {
            if (m_DialogError != null)
                return m_DialogError.toString();
            else
                return null;
        }

        String GetFailureURL()
        {

            if (m_DialogError != null)
                return ""; //m_DialogError.getFailingUrl(); //getFailingUrl is private
            else
                return null;
        }
    };


    class s3eFBRequest
    {

        class FBRequestListener implements RequestListener
        {
            public void onComplete(String response,Object state)
            {
                Log.d("app", "Got request complete, parsing JSON");
                try
                {
                    m_Response = Util.parseJson(response);
                    m_ResponseType = DICTIONARY_TYPE;
                }
                catch (Exception e)
                {
                    Log.d("app", "Parsing JSON failed, assuming response is string");
                    m_ResponseType = STRING_TYPE;
                }
                catch (FacebookError e)
                {
                    Log.d("app", "Parsing JSON failed, assuming response is string");
                    m_ResponseType = STRING_TYPE;
                }

                m_Complete = true;
                m_ResponseString = response;
                RequestCallback(s3eFBRequest.this, true);
            }

            public void onIOException(IOException e,Object state)
            {
                Log.d("app", "Got io execption");
                m_ErrorCode = 1;
                m_ErrorString = e.toString();
                RequestCallback(s3eFBRequest.this, false);
            }


            public void onFileNotFoundException(FileNotFoundException e,Object state)
            {
                Log.d("app", "Got file not found");
                m_ErrorCode = 1;
                m_ErrorString = e.toString();
                RequestCallback(s3eFBRequest.this, false);
            }


            public void onMalformedURLException(MalformedURLException e,Object state)
            {
                Log.d("app", "Got malformed url");
                m_ErrorCode = 1;
                m_ErrorString = e.toString();
                RequestCallback(s3eFBRequest.this, false);
            }

            public void onFacebookError(FacebookError e,Object state)
            {
                Log.d("app", "Got facebook error");
                m_ErrorCode = e.getErrorCode();
                m_ErrorString = e.toString();
                RequestCallback(s3eFBRequest.this, false);

            }
        }


        String m_Method;
        String m_HttpMethod;
        String m_Graph;
        String m_URL;
        Bundle m_Params;
        s3eFBSession m_Session;
        AsyncFacebookRunner m_FBRunner;

        boolean m_Complete;
        int m_ErrorCode;
        String m_ErrorString;
        JSONObject m_Response;
        String m_ResponseString;
        int m_ResponseType;

        s3eFBRequest(s3eFBSession session)
        {
            m_Params = new Bundle();
            m_Session = session;
            m_FBRunner = new AsyncFacebookRunner(session.m_Facebook);
        }

        void SetMethod(String method, String httpMethod)
        {
            m_Method = method;
            m_HttpMethod = httpMethod;
        }

        void SetGraph(String graph, String httpMethod)
        {
            m_Graph = graph;
            m_HttpMethod = httpMethod;
        }

        void AddParamString(String param, String value)
        {
            m_Params.putString(param, value);
        }

        void AddParamLong(String param, long value)
        {
            m_Params.putLong(param, value);
        }

        void SetURL(String URL, String httpMethod)
        {
            m_URL = URL;
            m_HttpMethod = httpMethod;
        }

        int Send()
        {
            if (m_URL != null)
            {
                Log.d("app", "Calling m_FBRunner.requestURL with url and http method");
                m_FBRunner.requestURL(m_URL, m_Params, m_HttpMethod, new FBRequestListener(),null);
            }
            else if (m_Method != null)
            {
                Log.d("app", "Calling m_FBRunner.request with method");
                m_Params.putString("method", m_Method);
                m_FBRunner.request(null, m_Params, m_HttpMethod, new FBRequestListener(),null);
            }
            else
            {
                Log.d("app", "Calling m_FBRunner.request with graph");
                m_FBRunner.request(m_Graph, m_Params, m_HttpMethod, new FBRequestListener(),null);
            }
            return S3E_RESULT_SUCCESS;
        }

        boolean GetError()
        {
            return m_ErrorCode != 0;
        }

        int GetErrorCode()
        {
            return m_ErrorCode;
        }

        String GetErrorString()
        {
            return m_ErrorString;
        }

        boolean GetComplete()
        {
            return m_Complete;
        }

        int GetResponseType()
        {
            return m_ResponseType;
        }

        String GetResponseAsString()
        {
            return m_ResponseString;
        }

        int GetResponseAsNumber()
        {
            //What does this need to do?
            return 0;
        }

        boolean ResponseDictionaryContainsItem(String item)
        {
            return m_Response.has(item);
        }


        String GetResponseDictionaryItemAsString(String item)
        {
            try
            {
                return m_Response.getString(item);
            }
            catch (Exception e)
            {
                return null;
            }
        }
    };


    public Object s3eFBInit(String appId)
    {
        s3eFBSession fb = new s3eFBSession(appId);
        return fb;

    }
    public int s3eFBTerminate(Object fbSession)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        fb.Logout();
        return 0;
    }
    public int s3eFBSession_Login(Object fbSession, String[] permissions)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        return fb.Login(permissions);
    }

    public int s3eFBSession_Logout(Object fbSession)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        return fb.Logout();
    }

    public boolean s3eFBSession_LoggedIn(Object fbSession)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        return fb.LoggedIn();
    }

    public String s3eFBSession_AccessToken(Object fbSession)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        return fb.m_Facebook.getAccessToken();
    }

    public Object s3eFBDialog_WithAction(Object fbSession, String action)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        s3eFBDialog dlg = new s3eFBDialog(fb, action);
        return dlg;
    }

    public int s3eFBDialog_Delete(Object dialog)
    {
        return 0;
    }

    public int s3eFBDialog_AddParamString(Object dialog, String name, String value)
    {
        s3eFBDialog dlg = (s3eFBDialog)dialog;
        dlg.AddParamString(name, value);
        return 0;
    }

    public int s3eFBDialog_AddParamNumber(Object dialog, String name, long value)
    {
        s3eFBDialog dlg = (s3eFBDialog)dialog;
        dlg.AddParamLong(name, value);
        return 0;
    }

    public int s3eFBDialog_Show(Object dialog)
    {
        s3eFBDialog dlg = (s3eFBDialog)dialog;
        return dlg.Show();
    }

    public boolean s3eFBDialog_Error(Object dialog)
    {
        s3eFBDialog dlg = (s3eFBDialog)dialog;
        return dlg.GetError();
    }

    public int s3eFBDialog_ErrorCode(Object dialog)
    {
        s3eFBDialog dlg = (s3eFBDialog)dialog;
        return dlg.GetErrorCode();
    }

    public String s3eFBDialog_ErrorString(Object dialog)
    {
        s3eFBDialog dlg = (s3eFBDialog)dialog;
        return dlg.GetErrorString();
    }

    public boolean s3eFBDialog_Complete(Object dialog)
    {
        s3eFBDialog dlg = (s3eFBDialog)dialog;
        return dlg.GetComplete();
    }

    public String s3eFBDialog_DidNotCompleteWithUrl(Object dialog)
    {
        s3eFBDialog dlg = (s3eFBDialog)dialog;
        return dlg.GetFailureURL();
    }

    public Object s3eFBRequest_WithMethodName(Object fbSession, String methodName, String httpMethod)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        s3eFBRequest ret = new s3eFBRequest(fb);
        ret.SetMethod(methodName, httpMethod);
        return ret;
    }

    public Object s3eFBRequest_WithGraphPath(Object fbSession, String graphPath, String httpMethod)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        s3eFBRequest ret = new s3eFBRequest(fb);
        ret.SetGraph(graphPath, httpMethod);
        return ret;
    }

    public Object s3eFBRequest_WithURL(Object fbSession, String url, String httpMethod)
    {
        s3eFBSession fb = (s3eFBSession)fbSession;
        s3eFBRequest ret = new s3eFBRequest(fb);
        ret.SetURL(url, httpMethod);
        return null;
    }

    public int s3eFBRequest_Delete(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return 0;
    }

    public int s3eFBRequest_AddParamString(Object request, String name, String value)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        req.AddParamString(name, value);
        return S3E_RESULT_SUCCESS;
    }

    public int s3eFBRequest_AddParamNumber(Object request, String name, long value)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        req.AddParamLong(name, value);
        return S3E_RESULT_SUCCESS;
    }

    public int s3eFBRequest_Send(Object request)
    {
        Log.d("app", "calling req send");
        s3eFBRequest req = (s3eFBRequest)request;
        return req.Send();
    }

    public boolean s3eFBRequest_Error(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetError();
    }

    public int s3eFBRequest_ErrorCode(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetErrorCode();
    }

    public String s3eFBRequest_ErrorString(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetErrorString();
    }

    public boolean s3eFBRequest_Complete(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetComplete();
    }

    public int s3eFBRequest_ResponseType(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetResponseType();
    }

    public String s3eFBRequest_ResponseRaw(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetResponseAsString();
    }

    public String s3eFBRequest_ResponseAsString(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetResponseAsString();

    }

    public long s3eFBRequest_ResponseAsNumber(Object request)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetResponseAsNumber();

    }

    public int s3eFBRequest_ResponseArrayCount(Object request)
    {
        return 0;
    }

    public String s3eFBRequest_ResponseArrayItemAsString(Object request, int index)
    {
        return null;
    }

    public boolean s3eFBRequest_ResponseDictionaryContainsItem(Object request, String key)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.ResponseDictionaryContainsItem(key);
    }

    public String s3eFBRequest_ResponseDictionaryItemAsString(Object request, String key)
    {
        s3eFBRequest req = (s3eFBRequest)request;
        return req.GetResponseDictionaryItemAsString(key);
    }

    public native void LoginCallback(Object session, boolean result);

    public native void DialogCallback(Object dialog, boolean result);

    public native void RequestCallback(Object request, boolean result);

}
