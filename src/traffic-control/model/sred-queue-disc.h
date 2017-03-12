/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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


#ifndef SRED_QUEUE_DISC_H
#define SRED_QUEUE_DISC_H
#define M 1000

#include "ns3/packet.h"
#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

class TraceContainer;

struct zombie
{
  uint32_t fid;         /* flow identifier */
  uint32_t count;       /* count of pkts of this flow in this zombie */
  Time timestamp;
};

/**
 * \ingroup traffic-control
 *
 * \brief A RED packet queue disc
 */
class SredQueueDisc : public QueueDisc
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief SredQueueDisc Constructor
   *
   * Create a SRED queue disc
   */
  SredQueueDisc ();

  /**
   * \brief Destructor
   *
   * Destructor
   */
  virtual ~SredQueueDisc ();

  /**
   * \brief Set the operating mode of this queue.
   *  Set operating mode
   *
   * \param mode The operating mode of this queue.
   */
  void SetMode (Queue::QueueMode mode);

  /**
   * \brief Get the encapsulation mode of this queue.
   * Get the encapsulation mode of this queue
   *
   * \returns The encapsulation mode of this queue.
   */
  Queue::QueueMode GetMode (void);
  uint32_t GetQueueSize (void);
  void SetQueueLimit (uint32_t lim);
  int64_t AssignStreams (int64_t stream);

protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);


private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void) const;
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);

  double CalculatePSred ();
  double CalculateSimplePZap (double pSred);
  double CalculateFullPZap (double pSred, int32_t hit);

  Queue::QueueMode m_mode;
  bool  m_isNs1Compat;
  uint32_t m_queueLimit;
  bool m_fullSred;                      /* boolean, TRUE:= full SRED, FALSE:= simple SRED */
  struct zombie zombielist[M];
  uint32_t m_listSize;
  double m_pOverwrite;
  double m_pHitFrequency;               /* hit frequency */
  double m_pMax;                        /* maximum drop probability */
  double m_alpha;

  Ptr<UniformRandomVariable> m_uv;
};

};  // namespace ns3

#endif // SRED_QUEUE_DISC_H
