/////////////////////////////////////////////////////////////////////////////////
//
// SourceFile: LongTermEvent.h
//
// Marlin Server: Internet server/client
// 
// Copyright (c) 2014-2022 ir. W.E. Huisman
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
#pragma once

//////////////////////////////////////////////////////////////////////////
//
// Long-Term-Events are cached by server and clients
// and stored in queue's in the server and the client
//

// Five types of events 
enum class EvtType
{
  EV_Open     = 1   // Once at max at the beginning of the stream
 ,EV_Message  = 2   // General message type (most messages)
 ,EV_Binary   = 3   // Binary messages only supported by WebSockets
 ,EV_Error    = 4   // Error state (can be intermixed in the stream)
 ,EV_Close    = 5   // Once at max at the end of the stream
};

// Policy of connections we would prefer
enum class EVChannelPolicy
{
  DP_NoPolicy       = 0       // No policy (yet)
 ,DP_Binary         = 1       // Binary messages needed (websocket only)
 ,DP_HighSecurity   = 2       // Only to the client (SSE only)
 ,DP_Disconnected   = 3       // Disconnected messages only (polling)
 ,DP_Immediate_S2C  = 4       // Immediate delivery server 2 client (websocket + SSE)
 ,DP_TwoWayMessages = 5       // Also client 2 server (websocket + polling)
 ,DP_NoSockets      = 6       // Running outside of IIS (SSE + polling)
 ,DP_SureDelivery   = 7       // All S2C channels (websocket + SSE + polling)
};

#define SENDER_RANDOM_NUMBER 0xADF74FF6

class LTEvent
{
public:
  LTEvent();
  LTEvent(EvtType p_type);

  static EvtType StringToEventType(XString p_type);
  static XString EventTypeToString(EvtType p_type);

  static EVChannelPolicy StringToChannelPolicy(XString p_policy);
  static XString ChannelPolicyToString(EVChannelPolicy p_policy);

  // DATA
  int     m_number { 0  };
  UINT64  m_sent   { 0L };
  EvtType m_type;
  XString m_typeName;
  XString m_payload;
};

// Application callback to handle our dissipated events
// Callback must free the event data with 'delete' !!
// Used at the CLIENT side of the event queues.
// (Server side uses LPFN_CALLBACK from the threadpool)
typedef void (*LPFN_EVENTCALLBACK)(void* p_object, LTEvent* p_event);
