// /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright  2011 Marcos Talau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (c) 1990-1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * PORT NOTE: This code was ported from ns-2 (queue/sred.cc).  Almost all
 * comments have also been ported from NS-2
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "sred-queue-disc.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/ipv6-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SredQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (SredQueueDisc);

TypeId
SredQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SredQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<SredQueueDisc> ()
    .AddAttribute ("Mode",
                   "Determines unit for QueueLimit",
                   EnumValue (Queue::QUEUE_MODE_PACKETS),
                   MakeEnumAccessor (&SredQueueDisc::SetMode),
                   MakeEnumChecker (Queue::QUEUE_MODE_BYTES, "QUEUE_MODE_BYTES",
                                    Queue::QUEUE_MODE_PACKETS, "QUEUE_MODE_PACKETS"))
    .AddAttribute ("QueueLimit",
                   "Queue limit in bytes/packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&SredQueueDisc::SetQueueLimit),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Ns1Compat",
                   "NS-1 compatibility",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SredQueueDisc::m_isNs1Compat),
                   MakeBooleanChecker ())
    .AddAttribute ("OverwriteProbability",
                   "Probability of overwriting zombie list",
                   DoubleValue (0.25),
                   MakeDoubleAccessor (&SredQueueDisc::m_pOverwrite),
                   MakeDoubleChecker <double> (0, 1))
    .AddAttribute ("MaximumDropProbability",
                   "maximum probability of dropping a packet",
                   DoubleValue (0.15),
                   MakeDoubleAccessor (&SredQueueDisc::m_pMax),
                   MakeDoubleChecker <double> (0, 1))
    .AddAttribute ("FullSRED",
                   "Simple SRED or Full SRED",
                   BooleanValue (false),
                   MakeBooleanAccessor (&SredQueueDisc::m_fullSred),
                   MakeBooleanChecker ())
  ;
  return tid;
}
SredQueueDisc::SredQueueDisc ()
  : QueueDisc ()
{
  NS_LOG_FUNCTION (this);
  m_uv = CreateObject<UniformRandomVariable> ();
}

SredQueueDisc::~SredQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
SredQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_uv = 0;
  QueueDisc::DoDispose ();
}

void
SredQueueDisc::SetMode (Queue::QueueMode mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_mode = mode;
}

Queue::QueueMode
SredQueueDisc::GetMode (void)
{
  NS_LOG_FUNCTION (this);
  return m_mode;
}

int64_t
SredQueueDisc::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_uv->SetStream (stream);
  return 1;
}

void
SredQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Initializing SRED params.");
  m_listSize = 0;
  m_pHitFrequency = 0;
  m_alpha = m_pOverwrite / M;
}

void
SredQueueDisc::SetQueueLimit (uint32_t lim)
{
  NS_LOG_FUNCTION (this << lim);
  m_queueLimit  = lim;
}

uint32_t
SredQueueDisc::GetQueueSize (void)
{
  NS_LOG_FUNCTION (this);
  if (GetMode () == Queue::QUEUE_MODE_BYTES)
    {
      return GetInternalQueue (0)->GetNBytes ();
    }
  else if (GetMode () == Queue::QUEUE_MODE_PACKETS)
    {
      return GetInternalQueue (0)->GetNPackets ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown SRED mode.");
    }
}

double
SredQueueDisc::CalculatePSred (void)
{
  int len, lim, lim3, lim6;
  len = GetQueueSize () * 536;
  lim = m_queueLimit * 536;
  lim3 = lim / 3;
  lim6 = lim / 6;
  if ((len >= lim3) && (len < lim))
    {
      return m_pMax;
    }
  else if ((len >= lim6) && (len < lim3))
    {
      return (m_pMax / 4);
    }
  else if ((len >= 0) && (len < lim6))
    {
      return 0;
    }
  return 1;
}

double
SredQueueDisc::CalculateSimplePZap (double pSred)
{
  double factor;
  factor = 256 * m_pHitFrequency;
  factor *= factor;
  factor = 1 / factor;
  if (factor > 1)
    {
      factor = 1;
    }
  return (pSred * factor);
}

double
SredQueueDisc::CalculateFullPZap (double pSred, int32_t hit)
{
  double pZap = CalculateSimplePZap (pSred);
  pZap *= (1 + (hit / m_pHitFrequency));
  return pZap;
}

Ptr<QueueDiscItem>
SredQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item = StaticCast<QueueDiscItem> (GetInternalQueue (0)->Dequeue ());

  NS_LOG_LOGIC ("Popped " << item);
  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}

bool
SredQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
 // Ipv6Header ipHeader;
  //item->GetPacket ()->PeekHeader (ipHeader);
      //  std::cout<<"m_queueLimit"<<m_queueLimit<<"\n";
       
 //uint32_t fid = ipHeader.GetFlowLabel();
 int32_t fid = Classify(item);
//std::cout<<"fid "<<fid<<"\n";
 if(fid == -1)
        {
            Drop (item);
          return false;
        }
  int32_t hit;
  uint32_t nQueued = GetQueueSize ();
  //std::cout<<"nQueued"<<nQueued<<"\n";
  if (m_listSize < M)
    { //std::cout<<"inside zombie"<<"GetPacketSize ()"<<std::cout<<item->GetPacketSize ()<<"\n";
      zombielist[m_listSize].fid = fid;
      zombielist[m_listSize].count = 0;
      zombielist[m_listSize].timestamp = Simulator::Now ();
      ++m_listSize;

      if ((GetMode () == Queue::QUEUE_MODE_PACKETS && nQueued < m_queueLimit)
          || (GetMode () == Queue::QUEUE_MODE_BYTES && nQueued + item->GetPacketSize () <= m_queueLimit))
        {
        //  std::cout<<"should enque";
          bool retval = GetInternalQueue (0)->Enqueue (item);
          return retval;

        }
      else
        {
          //std::cout<<"drp1\n";
          Drop (item);
          return false;
        }

    }
  else
    {
      uint32_t index  = m_uv->GetValue ();
      if (zombielist[index].fid == fid)
        {
          /* HIT */
          hit = 1;
          zombielist[index].count++;
          zombielist[index].timestamp = Simulator::Now ();
        }
      else
        {
          /* MISS */
          hit = 0;
          double u2 = m_uv->GetValue ();
          if (u2 < m_pOverwrite)
            {
              /* overwrite with random probability */
              zombielist[index].fid = fid;
              zombielist[index].count = 0;
              zombielist[index].timestamp = Simulator::Now ();
            }
        }

      /* update hit frequency */
      m_pHitFrequency = (1 - m_alpha) * m_pHitFrequency + m_alpha * hit;
    }
  double pSred = CalculatePSred ();

  /* calculate pZap */
  double pZap;
  if (m_fullSred)
    {
      pZap = CalculateFullPZap (pSred, hit);
    }
  else
    {
      pZap = CalculateSimplePZap (pSred);
    }
  double u1 = m_uv->GetValue ();
  if (u1 <= pZap)
    {

      /* drop the packet */
       // std::cout<<"drp2\n";
      Drop (item);
      return false;
    }
  else
    {
      /* enque the packet */
      if (nQueued >= m_queueLimit)
        {

          // drop
      // std::cout<<"drp3\n";
          Drop (item);
          return false;

        }
      else
        {
          // enque
          bool retval = GetInternalQueue (0)->Enqueue (item);
          return retval;
        }
    }
}

Ptr<const QueueDiscItem>
SredQueueDisc::DoPeek (void) const
{
  NS_LOG_FUNCTION (this);
  if (GetInternalQueue (0)->IsEmpty ())
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  Ptr<const QueueDiscItem> item = StaticCast<const QueueDiscItem> (GetInternalQueue (0)->Peek ());

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return item;
}

bool
SredQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("SredQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () == 0)
    {
      NS_LOG_ERROR ("SredQueueDisc needs atleast one packet filter");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // create a DropTail queue
      Ptr<Queue> queue = CreateObjectWithAttributes<DropTailQueue> ("Mode", EnumValue (m_mode));
      if (m_mode == Queue::QUEUE_MODE_PACKETS)
        {
          queue->SetMaxPackets (m_queueLimit);
        }
      else
        {
          queue->SetMaxBytes (m_queueLimit);
        }
      AddInternalQueue (queue);
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("SredQueueDisc needs 1 internal queue");
      return false;
    }

  if (GetInternalQueue (0)->GetMode () != m_mode)
    {
      NS_LOG_ERROR ("The mode of the provided queue does not match the mode set on the SredQueueDisc");
      return false;
    }

  if ((m_mode ==  Queue::QUEUE_MODE_PACKETS && GetInternalQueue (0)->GetMaxPackets () < m_queueLimit)
      || (m_mode ==  Queue::QUEUE_MODE_BYTES && GetInternalQueue (0)->GetMaxBytes () < m_queueLimit))
    {
      NS_LOG_ERROR ("The size of the internal queue is less than the queue disc limit");
      return false;
    }

  return true;
}

} // namespace ns3
