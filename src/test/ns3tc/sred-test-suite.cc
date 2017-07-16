/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Universita' degli Studi di Napoli Federico II
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
 * Authors: Pasquale Imputato <p.imputato@gmail.com>
 *          Stefano Avallone <stefano.avallone@unina.it>
*/

#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/sred-queue-disc.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-packet-filter.h"
#include "ns3/ipv4-queue-disc-item.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-header.h"
#include "ns3/ipv6-packet-filter.h"
#include "ns3/ipv6-queue-disc-item.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"

using namespace ns3;

/**
 * This class tests packets for which there is no suitable filter
 */
class SredQueueDiscNoSuitableFilter : public TestCase
{
public:
  SredQueueDiscNoSuitableFilter ();
  virtual ~SredQueueDiscNoSuitableFilter ();

private:
  virtual void DoRun (void);
};

SredQueueDiscNoSuitableFilter::SredQueueDiscNoSuitableFilter ()
  : TestCase ("Test packets that are not classified by any filter")
{
}

SredQueueDiscNoSuitableFilter::~SredQueueDiscNoSuitableFilter ()
{
}

void
SredQueueDiscNoSuitableFilter::DoRun (void)
{
  Ptr<SredQueueDisc> queueDisc = CreateObject<SredQueueDisc> ();
  Ptr<FqCoDelIpv4PacketFilter> filter = CreateObject<FqCoDelIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (filter);


  queueDisc->Initialize ();

  Ptr<Packet> p;
  p = Create<Packet> ();
  Ptr<Ipv6QueueDiscItem> item;
  Ipv6Header ipv6Header;
  Address dest;
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 0, "no flow queue should have been created");

  p = Create<Packet> (reinterpret_cast<const uint8_t*> ("hello, world"), 12);
  item = Create<Ipv6QueueDiscItem> (p, dest, 0, ipv6Header);
  queueDisc->Enqueue (item);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->GetNQueueDiscClasses (), 0, "no flow queue should have been created");

  Simulator::Destroy ();
}

/**
 * This class tests the IP flows separation and the packet limit
 *
*/

class SredQueueDiscAddPacketFromSameFlow : public TestCase
{
public:
  SredQueueDiscAddPacketFromSameFlow ();
  virtual ~SredQueueDiscAddPacketFromSameFlow ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SredQueueDisc> queue, Ipv4Header hdr);
};

SredQueueDiscAddPacketFromSameFlow::SredQueueDiscAddPacketFromSameFlow ()
  : TestCase ("Test IP flows separation and packet limit")
{
}

SredQueueDiscAddPacketFromSameFlow::~SredQueueDiscAddPacketFromSameFlow ()
{
}

void
SredQueueDiscAddPacketFromSameFlow::AddPacket (Ptr<SredQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
SredQueueDiscAddPacketFromSameFlow::DoRun (void)
{
  Ptr<SredQueueDisc> queueDisc = CreateObject<SredQueueDisc> ();  
  Ptr<FqCoDelIpv4PacketFilter> filter = CreateObject<FqCoDelIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (filter);
  queueDisc->Initialize ();
  
  queueDisc->SetQueueLimit (1024);
  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  
  for(uint32_t i=0;i<1024;i++)
{
  AddPacket (queueDisc, hdr);
}

  uint32_t numPacketsEnqueued = queueDisc->QueueDisc::GetNPackets ();
 // std::cout<<"Number of packets in the queue: "<<numPacketsEnqueued;
  //NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");
 
  Ptr<SredQueueDisc> queueDisc1 = CreateObject<SredQueueDisc> ();  
  Ptr<FqCoDelIpv4PacketFilter> filter1 = CreateObject<FqCoDelIpv4PacketFilter> ();
  queueDisc1->AddPacketFilter (filter1);
  queueDisc1->Initialize ();  
  queueDisc1->SetQueueLimit (1024);
 
 Ipv4Header hdr1;
  hdr1.SetPayloadSize (100);
  hdr1.SetSource (Ipv4Address ("10.10.1.1"));
  hdr1.SetDestination (Ipv4Address ("10.10.1.7"));
  hdr1.SetProtocol (7);

  Ipv4Header hdr2;
  hdr2.SetPayloadSize (100);
  hdr2.SetSource (Ipv4Address ("10.10.1.1"));
  hdr2.SetDestination (Ipv4Address ("10.10.1.6"));
  hdr2.SetProtocol (7);

  Ipv4Header hdr3;
  hdr3.SetPayloadSize (100);
  hdr3.SetSource (Ipv4Address ("10.10.1.1"));
  hdr3.SetDestination (Ipv4Address ("10.10.1.5"));
  hdr3.SetProtocol (7);

  Ipv4Header hdr4;
  hdr4.SetPayloadSize (100);
  hdr4.SetSource (Ipv4Address ("10.10.1.1"));
  hdr4.SetDestination (Ipv4Address ("10.10.1.4"));
  hdr4.SetProtocol (7);

  // Add the first packet
  for (uint32_t j=0;j<256;j++)
    {
      AddPacket (queueDisc1, hdr1);
      AddPacket (queueDisc1, hdr2);
      AddPacket (queueDisc1, hdr3);
      AddPacket (queueDisc1, hdr4);
    }

   uint32_t numPacketsEnqueued1 = queueDisc1->QueueDisc::GetNPackets ();
  //std::cout<<"Number of packets in the new queue : "<<numPacketsEnqueued1;
 
  NS_TEST_ASSERT_MSG_GT (numPacketsEnqueued, numPacketsEnqueued1, "unexpected number of packets in the flow queue");

  Simulator::Destroy ();
}


 // This class tests the deficit per flow
 
class SredEnqueue : public TestCase
{
public:
  SredEnqueue ();
  virtual ~SredEnqueue ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SredQueueDisc> queue, Ipv4Header hdr);
};

SredEnqueue::SredEnqueue ()
  : TestCase ("Test Sred Enqueue")
{
}

SredEnqueue::~SredEnqueue ()
{
}

void
SredEnqueue::AddPacket (Ptr<SredQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
SredEnqueue::DoRun (void)
{
  Ptr<SredQueueDisc> queueDisc = CreateObject<SredQueueDisc> ();
  Ptr<FqCoDelIpv6PacketFilter> ipv6Filter = CreateObject<FqCoDelIpv6PacketFilter> ();
  Ptr<FqCoDelIpv4PacketFilter> ipv4Filter = CreateObject<FqCoDelIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->Initialize ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add a packet from the first flow
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");

  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 7, "unexpected number of packets in the queue disc");
 
  Simulator::Destroy ();
}


 // This class tests the deficit per flow
 
class SredDequeue : public TestCase
{
public:
  SredDequeue ();
  virtual ~SredDequeue ();

private:
  virtual void DoRun (void);
  void AddPacket (Ptr<SredQueueDisc> queue, Ipv4Header hdr);
};

SredDequeue::SredDequeue ()
  : TestCase ("Test Sred Dequeue")
{
}

SredDequeue::~SredDequeue ()
{
}

void
SredDequeue::AddPacket (Ptr<SredQueueDisc> queue, Ipv4Header hdr)
{
  Ptr<Packet> p = Create<Packet> (100);
  Address dest;
  Ptr<Ipv4QueueDiscItem> item = Create<Ipv4QueueDiscItem> (p, dest, 0, hdr);
  queue->Enqueue (item);
}

void
SredDequeue::DoRun (void)
{
  Ptr<SredQueueDisc> queueDisc = CreateObject<SredQueueDisc> ();
  Ptr<FqCoDelIpv6PacketFilter> ipv6Filter = CreateObject<FqCoDelIpv6PacketFilter> ();
  Ptr<FqCoDelIpv4PacketFilter> ipv4Filter = CreateObject<FqCoDelIpv4PacketFilter> ();
  queueDisc->AddPacketFilter (ipv6Filter);
  queueDisc->AddPacketFilter (ipv4Filter);

  queueDisc->Initialize ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");
  

  Ipv4Header hdr;
  hdr.SetPayloadSize (100);
  hdr.SetSource (Ipv4Address ("10.10.1.1"));
  hdr.SetDestination (Ipv4Address ("10.10.1.2"));
  hdr.SetProtocol (7);

  // Add a packet from the first flow
  AddPacket (queueDisc, hdr);
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 1, "unexpected number of packets in the queue disc");

  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  AddPacket (queueDisc, hdr);
  
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 6, "unexpected number of packets in the queue disc");

  queueDisc->Dequeue ();
  queueDisc->Dequeue ();
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 3, "unexpected number of packets in the queue disc");

  queueDisc->Dequeue ();
  queueDisc->Dequeue ();
  queueDisc->Dequeue ();
  NS_TEST_ASSERT_MSG_EQ (queueDisc->QueueDisc::GetNPackets (), 0, "unexpected number of packets in the queue disc");

  
 
  Simulator::Destroy ();
}



class SredQueueDiscTestSuite : public TestSuite
{
public:
  SredQueueDiscTestSuite ();
};

SredQueueDiscTestSuite::SredQueueDiscTestSuite ()
  : TestSuite ("sred-test", UNIT)
{
  AddTestCase (new SredQueueDiscNoSuitableFilter, TestCase::QUICK);
  AddTestCase (new SredQueueDiscAddPacketFromSameFlow, TestCase::QUICK);
  AddTestCase (new SredEnqueue, TestCase::QUICK);
  AddTestCase (new SredDequeue, TestCase::QUICK);
}

static SredQueueDiscTestSuite SredQueueDiscTestSuite;
