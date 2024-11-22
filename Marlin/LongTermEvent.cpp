/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: LongTermEvent.cpp
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2024 ir. W.E. Huisman
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include "stdafx.h"
#include "LongTermEvent.h"

#ifdef _AFX
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

LTEvent::LTEvent()
        :m_type(EvtType::EV_Message)
{
}

LTEvent::LTEvent(EvtType p_type)
        :m_type(p_type)
{
}

EvtType
LTEvent::StringToEventType(XString p_type)
{
  if(p_type.Compare(_T("open"))    == 0) return EvtType::EV_Open;
  if(p_type.Compare(_T("message")) == 0) return EvtType::EV_Message;
  if(p_type.Compare(_T("binary"))  == 0) return EvtType::EV_Binary;
  if(p_type.Compare(_T("error"))   == 0) return EvtType::EV_Error;
  if(p_type.Compare(_T("close"))   == 0) return EvtType::EV_Close;

  // Default: It's just a message
  return EvtType::EV_Message;
}

XString 
LTEvent::EventTypeToString(EvtType p_type)
{
  switch (p_type)
  {
    case EvtType::EV_Open:    return _T("open");
    case EvtType::EV_Message: return _T("message");
    case EvtType::EV_Binary:  return _T("binary");
    case EvtType::EV_Error:   return _T("error");
    case EvtType::EV_Close:   return _T("close");
  }
  return _T("");
}

EVChannelPolicy 
LTEvent::StringToChannelPolicy(XString p_policy)
{
  if(p_policy.CompareNoCase(_T("No policy"))        == 0) return EVChannelPolicy::DP_NoPolicy;
  if(p_policy.CompareNoCase(_T("Binary"))           == 0) return EVChannelPolicy::DP_Binary;
  if(p_policy.CompareNoCase(_T("High security"))    == 0) return EVChannelPolicy::DP_HighSecurity;
  if(p_policy.CompareNoCase(_T("Disconnected"))     == 0) return EVChannelPolicy::DP_Disconnected;
  if(p_policy.CompareNoCase(_T("Immediate S2C"))    == 0) return EVChannelPolicy::DP_Immediate_S2C;
  if(p_policy.CompareNoCase(_T("Two way messages")) == 0) return EVChannelPolicy::DP_TwoWayMessages;
  if(p_policy.CompareNoCase(_T("Sure delivery"))    == 0) return EVChannelPolicy::DP_SureDelivery;

  return EVChannelPolicy::DP_NoPolicy;
}

XString 
LTEvent::ChannelPolicyToString(EVChannelPolicy p_policy)
{
  switch(p_policy)
  {
    case EVChannelPolicy::DP_NoPolicy:        return _T("No policy");
    case EVChannelPolicy::DP_Binary:          return _T("Binary");
    case EVChannelPolicy::DP_HighSecurity:    return _T("High security");
    case EVChannelPolicy::DP_Disconnected:    return _T("Disconnected");
    case EVChannelPolicy::DP_Immediate_S2C:   return _T("Immediate S2C");
    case EVChannelPolicy::DP_TwoWayMessages:  return _T("Two way messages");
    case EVChannelPolicy::DP_SureDelivery:    return _T("Sure delivery");
  }
  return _T("");
}
