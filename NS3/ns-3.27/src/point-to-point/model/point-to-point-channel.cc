/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2008 University of Washington
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
 */

#include "point-to-point-channel.h"
#include "point-to-point-net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 { 

NS_LOG_COMPONENT_DEFINE ("PointToPointChannel");

NS_OBJECT_ENSURE_REGISTERED (PointToPointChannel);

TypeId 
PointToPointChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PointToPointChannel")
    .SetParent<Channel> ()
    .SetGroupName ("PointToPoint")
    .AddConstructor<PointToPointChannel> ()
    .AddAttribute ("Delay", "Propagation delay through the channel",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&PointToPointChannel::m_delay),
                   MakeTimeChecker ())
    .AddAttribute ("isDelayChange", "the mode of random change Delay",
		   	   	      BooleanValue (false),
					  MakeBooleanAccessor (&PointToPointChannel::isDelayChange),
					  MakeBooleanChecker ())
	.AddAttribute ("islossChange", "the mode of random change loss",
		   	   	      BooleanValue (false),
					  MakeBooleanAccessor (&PointToPointChannel::islossChange),
					  MakeBooleanChecker ())
	.AddAttribute ("isBwChange", "the mode of random change Bandwidth",
		   	   	      BooleanValue (false),
					  MakeBooleanAccessor (&PointToPointChannel::isBwChange),
					  MakeBooleanChecker ())
	.AddAttribute ("RandomInterval", 
                   "The time update random environment",
                   TimeValue (Seconds (0x3f3f3f3f)),
                   MakeTimeAccessor (&PointToPointChannel::m_interval),
                   MakeTimeChecker ())
  .AddAttribute ("poissonDelay", "random change Delay with poisson distribution",
                  BooleanValue (false),
            MakeBooleanAccessor (&PointToPointChannel::poissonDelay),
            MakeBooleanChecker ())
  .AddTraceSource ("TxRxPointToPoint",
                   "Trace source indicating transmission of packet "
                   "from the PointToPointChannel, used by the Animation "
                   "interface.",
                   MakeTraceSourceAccessor (&PointToPointChannel::m_txrxPointToPoint),
                   "ns3::PointToPointChannel::TxRxAnimationCallback")
  ;
  return tid;
}

//
// By default, you get a channel that 
// has an "infitely" fast transmission speed and zero delay.
PointToPointChannel::PointToPointChannel()
  :
    Channel (),
    m_delay (Seconds (0.)),
    m_nDevices (0),
	m_interval(0.5) 
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
PointToPointChannel::Attach (Ptr<PointToPointNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  NS_ASSERT_MSG (m_nDevices < N_DEVICES, "Only two devices permitted");
  NS_ASSERT (device != 0);

  m_link[m_nDevices++].m_src = device;
//
// If we have both devices connected to the channel, then finish introducing
// the two halves and set the links to IDLE.
//
  if (m_nDevices == N_DEVICES)
    {
	if(isDelayChange || isBwChange || islossChange)
	{
    baseRTT = m_delay.GetSeconds()*1000000; //us
		if(!m_RandomDelay){
			m_RandomDelay=CreateObject<UniformRandomVariable> ();
		}
		// ScheduleDelayChange(); 
    Simulator::Schedule (10*m_delay, &PointToPointChannel::ScheduleDelayChange, this);
	}
		//device->ScheduleRateChange();

      m_link[0].m_dst = m_link[1].m_src;
      m_link[1].m_dst = m_link[0].m_src;
      m_link[0].m_state = IDLE;
      m_link[1].m_state = IDLE;
    }
}

bool
PointToPointChannel::TransmitStart (
  Ptr<const Packet> p,
  Ptr<PointToPointNetDevice> src,
  Time txTime)
{
  NS_LOG_FUNCTION (this << p << src);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);

  uint32_t wire = src == m_link[0].m_src ? 0 : 1;

  Simulator::ScheduleWithContext (m_link[wire].m_dst->GetNode ()->GetId (),
                                  txTime + m_delay, &PointToPointNetDevice::Receive,
                                  m_link[wire].m_dst, p->Copy ());

  // Call the tx anim callback on the net device
  m_txrxPointToPoint (p, src, m_link[wire].m_dst, txTime, txTime + m_delay);
  return true;
}

uint32_t 
PointToPointChannel::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_nDevices;
}

Ptr<PointToPointNetDevice>
PointToPointChannel::GetPointToPointDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (i < 2);
  return m_link[i].m_src;
}

Ptr<NetDevice>
PointToPointChannel::GetDevice (uint32_t i) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return GetPointToPointDevice (i);
}

Time
PointToPointChannel::GetDelay (void) const
{
  return m_delay;
}

Ptr<PointToPointNetDevice>
PointToPointChannel::GetSource (uint32_t i) const
{
  return m_link[i].m_src;
}

Ptr<PointToPointNetDevice>
PointToPointChannel::GetDestination (uint32_t i) const
{
  return m_link[i].m_dst;
}

bool
PointToPointChannel::IsInitialized (void) const
{
  NS_ASSERT (m_link[0].m_state != INITIALIZING);
  NS_ASSERT (m_link[1].m_state != INITIALIZING);
  return true;
}

//wujia 2020 0512
void
PointToPointChannel::setRandomDelay (Ptr<UniformRandomVariable> pra1)
{
	m_RandomDelay=pra1;
}

void
PointToPointChannel::ScheduleDelayChange_tw (void)
{
    m_link[0].m_src->SetDataRate(DataRate (12* 1000 * 1000));
    m_link[1].m_src->SetDataRate(DataRate (12 * 1000 * 1000));
    std::cout<<" ScheduleRateChange "<<Simulator::Now().GetSeconds()<<" value "<<12<<std::endl;
}

uint32_t PointToPointChannel::poissonGenerator(double lambda)
{
    double u = m_RandomDelay->GetValue (0,10000)/10000;
    double p = exp(-lambda);
    double f = p;
    uint32_t x = 0;

    while (f < u)
    {
        
        x++;
        p*=(lambda/(double)x);
        f+=p;

        // std::cout<<"u "<<u<<" x "<<x<<" f "<<f<<" lambda "<<lambda<<std::endl;
    }
    return x;
}

void
PointToPointChannel::ScheduleDelayChange (void)
{

  // //for heter

  //   //m_bps=DataRate (value * 1000 * 1000);
  //   m_link[0].m_src->SetDataRate(DataRate (36 * 1000 * 1000));
  //   m_link[1].m_src->SetDataRate(DataRate (36 * 1000 * 1000));
  //   std::cout<<" ScheduleRateChange "<<Simulator::Now().GetSeconds()<<" value "<<8<<std::endl;

  //   // Simulator::Schedule (Time(Seconds(0.01)), &PointToPointChannel::ScheduleDelayChange_tw, this);

  // return ;

  // /////////

  if(isDelayChange && poissonDelay)
  {
    
    double value=m_RandomDelay->GetValue (0,100);

    // if(baseRTT==m_delay.GetSeconds()*1000000)
    if(value>50)
    {
      // double value = baseRTT*1e-3 + poissonGenerator(2*baseRTT/2*1e-3);//ms
      value = baseRTT*1e-2 + poissonGenerator(2*baseRTT/10*1e-2);//0.1ms
      m_delay = MicroSeconds(value*1e2);

      // value = poissonGenerator(2*baseRTT/10*1e-3);
      do
      {
        value = poissonGenerator(4*baseRTT/100*1*1e-2);
      }while(value <= 0);

      m_interval = MicroSeconds(value*1e2);

    }
    else
    {
      m_delay=MicroSeconds(baseRTT);

      do
      {
        value = poissonGenerator(4*baseRTT/100*99*1e-2);
      }while(value <= 0);

      m_interval = MicroSeconds(value*1e2);


    }
    
    std::cout<<" ScheduleDelayChange "<<Simulator::Now().GetSeconds()<<" rtt "<<2*(baseRTT*1e-3+m_delay.GetSeconds()*1e3) << " interval " << value*1e-1 <<std::endl;

    Simulator::Schedule (m_interval, &PointToPointChannel::ScheduleDelayChange, this);

    return ;
  }

  double value = m_RandomDelay->GetValue (0,45);//s
  if(isDelayChange){
	  m_delay = MicroSeconds (value*1000);//us
	  std::cout<<" ScheduleDelayChange "<<Simulator::Now().GetSeconds()<<" value "<<value*2+5<<std::endl;
  }
  if(islossChange){
	  value = m_RandomDelay->GetValue (0,100)/10000;
	  //m_receiveErrorModel = PointerValue (em);
	  m_link[1].m_src->Setlossrate(value);
	  std::cout<<" SchedulelossChange "<<Simulator::Now().GetSeconds()<<" value "<<value*100<<" %"<<std::endl;
  }
  if(isBwChange){
	  value = m_RandomDelay->GetValue (10,100);
	  //m_bps=DataRate (value * 1000 * 1000);
	  m_link[0].m_src->SetDataRate(DataRate (value * 1000 * 1000));
	  m_link[1].m_src->SetDataRate(DataRate (value * 1000 * 1000));
	  std::cout<<" ScheduleRateChange "<<Simulator::Now().GetSeconds()<<" value "<<value<<std::endl;
  }
  if(isDelayChange || isBwChange || islossChange){
	  Time tNext (m_interval);  
	  Simulator::Schedule (tNext, &PointToPointChannel::ScheduleDelayChange, this);
  }
}


} // namespace ns3
