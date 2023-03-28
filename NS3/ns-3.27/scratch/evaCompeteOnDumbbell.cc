/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * pld ns-3 version of basic1: single sender through a router
 * To run: ./waf --run basic1
 * To run and set a command-line argument: ./waf --run "basic1 --runtime=10"
 * To enable logging (to stderr), set this in the environment: NS_LOG=TcpReno=level_info
 * From http://intronetworks.cs.luc.edu/current/html/ns3.html
 */



#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/traffic-control-module.h"
#include<cmath>
#include <sstream>

using namespace ns3;

std::string fileNameRoot = "../Testdata/";

void CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " <<  newCwnd/1448.0 << std::endl;
}

static void
TraceCwnd (int flowNum)    // Trace changes to the congestion window
{
  AsciiTraceHelper ascii;

  std::stringstream stemp2;
  if (true){
	  for(int i=0;i<(flowNum+1)/2;i++ ){
		  std::stringstream stemp1;
		  std::stringstream stemp2;
		  stemp1<<fileNameRoot<<i<<".cwnd";
		  Ptr<OutputStreamWrapper> stream1 = ascii.CreateFileStream (stemp1.str());
		  stemp2<<"/NodeList/0/$ns3::TcpL4Protocol/SocketList/"<<i<<"/CongestionWindow";
		  Config::ConnectWithoutContext (stemp2.str(),MakeBoundCallback (&CwndChange,stream1));
	  }
	  for(int i=0;i<flowNum/2;i++ ){
		  std::stringstream stemp1;
		  std::stringstream stemp2;
		  stemp1<<fileNameRoot<<i+flowNum/2<<".cwnd";
		  Ptr<OutputStreamWrapper> stream1 = ascii.CreateFileStream (stemp1.str());
		  stemp2<<"/NodeList/1/$ns3::TcpL4Protocol/SocketList/"<<i<<"/CongestionWindow";
		  Config::ConnectWithoutContext (stemp2.str(),MakeBoundCallback (&CwndChange,stream1));
		}
  }
}

static void
TraceThr (int flowNum,double old_byte,double old_t,ApplicationContainer *sinkAppA,Ptr<OutputStreamWrapper> stream)    // Trace Throughput
{
  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkAppA->Get (0));
  double cur_time=Simulator::Now ().GetSeconds ();
  *stream->GetStream () << cur_time <<" ";
  double l_sum=0;
  for(int i=0;i<flowNum;i++){
	  sink1 = DynamicCast<PacketSink> (sinkAppA->Get (i));
	  l_sum+=sink1->GetTotalRx ();
  }
  *stream->GetStream () << (l_sum-old_byte)/(cur_time-old_t)<<std::endl;//byte/s
  Simulator::Schedule(Seconds(1.0),&TraceThr,flowNum,l_sum,cur_time,sinkAppA,stream);
  
}

static void
RxDrop (Ptr<const Packet> p)
{
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

void QueueChange (Ptr<OutputStreamWrapper> stream, uint32_t oldQ, uint32_t newQ)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " <<  oldQ <<" "<< newQ <<std::endl;
}
//another trace way is simulate ns2

static void
TraceQueue (double N)    // Trace changes to the congestion window
{
//   std::cout <<n<<std::endl;
  AsciiTraceHelper ascii;
  std::stringstream stemp1;
  std::stringstream stemp2;

  for(int i=2;i<3;i++){
	  std::stringstream stemp1;
	  std::stringstream stemp2;
	  stemp1<<fileNameRoot<<"PDCC"<<".q";
	  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (stemp1.str());
	  stemp2<<"/NodeList/"<<2<<"/DeviceList/"<<2<<"/$ns3::PointToPointNetDevice/TxQueue/PacketsInQueue";
	  Config::ConnectWithoutContext (stemp2.str(), MakeBoundCallback (&QueueChange,stream));
  }
}

NS_LOG_COMPONENT_DEFINE ("Copatest");
int 
main (int argc, char *argv[])
{
  std::cout<<"testcopa"<<std::endl;

  //LogComponentEnable ("TcpSocketBase", LOG_LEVEL_INFO); 

  int tcpSegmentSize = 1448;//Bytes

   
  Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold",UintegerValue (2*tcpSegmentSize));     
  unsigned int runtime = 4;   // seconds
  double delayAR = 5;            // ms
  double delayRB = 5;            // ms
  double bottleneckBW= 100;    // Mbps
  double fastBW = 100;          // Mbps
  uint32_t queuesize = 600;
  uint32_t maxBytes = 0; // 0 means "unlimited"
  double lamuda=2;
  double assistPra=0.0;
  int flowNum = 2; 
  double postfix=flowNum;

  bool iscopaModify = true,rand_loss=false,rand_delay=false,rand_BW=false,poisson_delay=true;
  bool iftrace=false, if_comp=true;
  double rand_interval=5;
  double Rc=33;
  double Bd=10;
  int buffer=2000;
  
  double delay=10;
  double cycle=1;
  
  int CC_mode=1,attackerCC=5;// 0-newReno 1-Copa+ 2-Copa 3-BBR 4-Copa+-origin 7-Copa+-strawman 5-Cubic

  uint32_t attackTime = 0, atteckerExit = 4;



  // Here, we define command line options overriding some of the above.  
  CommandLine cmd;
  cmd.AddValue ("trace", "gener trace file", iftrace);
  cmd.AddValue ("runtime", "How long the applications should send data", runtime);
  cmd.AddValue ("delayRB", "Delay on the R--B link, in ms", delay);
  cmd.AddValue ("cycle", "T for PDCC, in s", cycle);
  cmd.AddValue ("CC_mode", "T for PDCC, in s", CC_mode);
  cmd.AddValue ("attackerCC", "attackerCC", attackerCC);
  cmd.AddValue ("attackTime", "attackTime", attackTime);
  cmd.AddValue ("atteckerExit", "atteckerExit", atteckerExit);
  cmd.AddValue ("queuesize", "queue size at R", queuesize);
  cmd.AddValue ("tcpSegmentSize", "TCP segment size", tcpSegmentSize);
  cmd.AddValue ("iscopaModify", "copa Modify", iscopaModify);
  cmd.AddValue ("rand_loss", "randomly changes loss rate", rand_loss);
  cmd.AddValue ("rand_BW", "randomly changes bandwidth", rand_BW);
  cmd.AddValue ("rand_delay", "randomly changes delay", rand_delay);
  cmd.AddValue ("poisson_delay", "randomly changes delay with poisson distribution", poisson_delay);
  cmd.AddValue ("rand_interval", "randomly changes interval", rand_interval);
  cmd.AddValue ("flowNum", "flow Number", flowNum);
  cmd.AddValue ("lamuda", "flow packet in copa", lamuda);
  cmd.AddValue ("assistPra", "assist Number", assistPra);
  cmd.AddValue ("BW", "bandwidth", bottleneckBW);
  cmd.AddValue ("Rc", "Rc", Rc);
  cmd.AddValue ("Bd", "Bd", Bd);
  cmd.AddValue ("Buffer", "RcvBufSize", buffer);
  cmd.AddValue ("postfix", "post ffix", postfix);
  cmd.AddValue ("if_comp", "enable competitive mode?", if_comp);
  cmd.Parse (argc, argv);

  //delta=1.0/delta;
 
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (1));

  std::string TCP_PROTOCOL;
  switch(CC_mode) //chose tested scheme
  {
	case 0:
		TCP_PROTOCOL="ns3::TcpNewReno";
		fileNameRoot+="NewReno/";
		break;
	case 1:  //Copa+
		TCP_PROTOCOL="ns3::TcpCopa";
    fileNameRoot+="Copa+/";
    LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO); 
    Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (true));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (1));
    //Config::SetDefault ("ns3::TcpSocketBase::Pdcc", BooleanValue (1));
		break;
	case 2:  //Copa
		TCP_PROTOCOL="ns3::TcpCopa";
		fileNameRoot+="Copa/";
		LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO);
		Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (false));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (0));
		break;
	case 3:
		TCP_PROTOCOL="ns3::TcpBbr";
		fileNameRoot+="BBR/";
		LogComponentEnable ("TcpBbr", LOG_LEVEL_INFO);
		break;
	case 4:
		TCP_PROTOCOL="ns3::TcpCopa";
		fileNameRoot+="Copa+-origin/";
		LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO);
		Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (true));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (0));
		break;
	case 5:
		TCP_PROTOCOL="ns3::TcpCubic";
		fileNameRoot+="Cubic/";
		LogComponentEnable ("TcpCubic", LOG_INFO);
		break;
	case 6:
		TCP_PROTOCOL="ns3::TcpNewReno";
		fileNameRoot+="NewReno/";
		//LogComponentEnable ("TcpNewReno", LOG_INFO);
		break;
  case 7:
    TCP_PROTOCOL="ns3::TcpCopa";
    fileNameRoot+="Copa+-strawman/";
    LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO);
    Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (true));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (2));
    break;
	default:
		TCP_PROTOCOL="ns3::TcpCopa";
		fileNameRoot+="Copa+/";
		LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO); 
		Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (true));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (1));
  }

  std::string attacker_PROTOCOL;
  switch(attackerCC)
  {
  case 0:
    attacker_PROTOCOL="ns3::TcpNewReno";
    fileNameRoot+="vsNewReno/";
    break;
  case 1:  //Copa+
    attacker_PROTOCOL="ns3::TcpCopa";
    fileNameRoot+="vsCopa+/";
    LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO); 
    Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (true));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (1));
    //Config::SetDefault ("ns3::TcpSocketBase::Pdcc", BooleanValue (1));
    break;
  case 2:  //Copa
    attacker_PROTOCOL="ns3::TcpCopa";
    fileNameRoot+="vsCopa/";
    LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO);
    Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (false));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (0));
    break;
  case 3:
    attacker_PROTOCOL="ns3::TcpBbr";
    fileNameRoot+="vsBBR/";
    LogComponentEnable ("TcpBbr", LOG_LEVEL_INFO);
    break;
  case 4:
    attacker_PROTOCOL="ns3::TcpCopa";
    fileNameRoot+="vsCopa+-origin/";
    LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO);
    Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (true));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (0));
    break;
  case 5:
    attacker_PROTOCOL="ns3::TcpCubic";
    fileNameRoot+="vsCubic/";
    LogComponentEnable ("TcpCubic", LOG_INFO);
    break;
  case 6:
    attacker_PROTOCOL="ns3::TcpNewReno";
    fileNameRoot+="vsNewReno/";
    //LogComponentEnable ("TcpNewReno", LOG_INFO);
    break;
  case 7:
    attacker_PROTOCOL="ns3::TcpCopa";
    fileNameRoot+="vsCopa+-strawman/";
    LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO);
    Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (true));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (2));
    break;
  default:
    attacker_PROTOCOL="ns3::TcpCopa";
    fileNameRoot+="vsCopa+/";
    LogComponentEnable ("TcpCopa", LOG_LEVEL_INFO); 
    Config::SetDefault ("ns3::TcpCopa::iscopaModify", BooleanValue (true));
    Config::SetDefault ("ns3::TcpCopa::enableComp", BooleanValue (if_comp));
    Config::SetDefault ("ns3::TcpCopa::cpVersion", UintegerValue (1));
  }
  
  
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcpSegmentSize));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (true));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (tcpSegmentSize*2000));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (tcpSegmentSize*buffer));
  Config::SetDefault ("ns3::PfifoFastQueueDisc::Limit", UintegerValue (1));
  //Config::SetDefault ("ns3::PointToPointChannel::isDelayChange", BooleanValue (false));
  //Config::SetDefault ("ns3::PointToPointChannel::islossChange", BooleanValue (false));
  //Config::SetDefault ("ns3::PointToPointChannel::isBwChange", BooleanValue (false));
  //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue (TCP_PROTOCOL));

  delayRB=delay/4;
  delayAR=delayRB;
  fastBW=bottleneckBW;


  std::cout<<iscopaModify<<std::endl;

  Ptr<Node> A1 = CreateObject<Node> ();
  Ptr<Node> A2 = CreateObject<Node> ();
  Ptr<Node> R = CreateObject<Node> ();
  Ptr<Node> B = CreateObject<Node> ();

  NetDeviceContainer devAR1, devAR2,devRB;
  PointToPointHelper AR1,AR2, RB;

  // create point-to-point link from A to R
  AR1.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (fastBW * 1000 * 1000)));
  AR1.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delayAR*1000)));
  AR1.SetQueue("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_PACKETS"), "MaxPackets", UintegerValue(queuesize));
  devAR1 = AR1.Install(A1, R);
   
  //Ptr<PointToPointChannel> ptpC=devAR1.Get(0)->GetChannel()->GetObject<PointToPointChannel>();
  //ptpC->setRandomDelay(uv);  

  AR2.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (fastBW * 1000 * 1000)));
  AR2.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delayAR*1000)));
  AR2.SetQueue("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_PACKETS"), "MaxPackets", UintegerValue(queuesize));
  devAR2 = AR2.Install(A2, R);

  // create point-to-point link from R to B
  RB.SetDeviceAttribute ("DataRate", DataRateValue (DataRate (bottleneckBW * 1000 * 1000)));
  //RB.SetDeviceAttribute ("isRandomChange", BooleanValue(rand_BW));
  //RB.SetDeviceAttribute ("isLossChange", BooleanValue(rand_loss));
  //RB.SetDeviceAttribute ("RandomInterval", TimeValue (Seconds (5)));
  RB.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (delayRB*1000)));
  RB.SetQueue("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_PACKETS"), "MaxPackets", UintegerValue(queuesize));

  /*------Danamic links with modification in p-t-p channel-----------*/

  RB.SetChannelAttribute ("isDelayChange", BooleanValue(rand_delay));
  RB.SetChannelAttribute ("islossChange", BooleanValue(rand_loss));
  RB.SetChannelAttribute ("isBwChange", BooleanValue(rand_BW));
  RB.SetChannelAttribute ("poissonDelay", BooleanValue(poisson_delay));
  RB.SetChannelAttribute ("RandomInterval", TimeValue (Seconds (rand_interval)));

  /*----------------------------------------------------------------*/

  
  devRB = RB.Install(R,B);
  

  

  /* Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.0005));
  RateErrorModel::ErrorUnit eunit=RateErrorModel::ERROR_UNIT_PACKET;
  em->SetUnit(eunit);
  devRB.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em)); */
  devRB.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

  InternetStackHelper internet;
  internet.Install (A1);
  internet.Install (A2);
  internet.Install (R);
  internet.Install (B);

  // Assign IP addresses
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ipv4Interfaces;
  ipv4Interfaces.Add(ipv4.Assign (devAR1));
  ipv4.SetBase ("10.0.1.0", "255.255.255.0");
  ipv4Interfaces.Add(ipv4.Assign (devAR2));
  ipv4.SetBase ("10.0.2.0", "255.255.255.0");
  ipv4Interfaces.Add(ipv4.Assign(devRB));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // gets node A's IPv4 subsystem
  Ptr<Ipv4> A14 = A1->GetObject<Ipv4>();
  Ptr<Ipv4> A24 = A2->GetObject<Ipv4>();
  Ptr<Ipv4> B4 = B->GetObject<Ipv4>();
  Ptr<Ipv4> R4 = R->GetObject<Ipv4>();
  Ipv4Address Aaddr1 = A14->GetAddress(1,0).GetLocal();
  Ipv4Address Aaddr2 = A24->GetAddress(1,0).GetLocal();
  Ipv4Address Baddr = B4->GetAddress(1,0).GetLocal();
  Ipv4Address Raddr = R4->GetAddress(1,0).GetLocal();

  std::cout << "A1's address: " << Aaddr1 << std::endl;
  std::cout << "A2's address: " << Aaddr2 << std::endl;
  std::cout << "B's address: " << Baddr << std::endl;
  std::cout << "R's #1 address: " << Raddr << std::endl;
  std::cout << "R's #2 address: " << R4->GetAddress(2,0).GetLocal() << std::endl;


  TrafficControlHelper tch; // solve the droping problems
  tch.Uninstall (devAR1);
  tch.Uninstall (devAR2);
  tch.Uninstall (devRB);

  // create flowNum sink on B


  uint16_t Bport = 80;
  ApplicationContainer sinkAppA ;
  std::vector<Address> sinkAddrs;
  for(int i=0;i<flowNum;i++){
	  Address sinkAaddr(InetSocketAddress (Ipv4Address::GetAny (), Bport+i));
	  PacketSinkHelper sinkA ("ns3::TcpSocketFactory", sinkAaddr);
	  sinkAppA.Add(sinkA.Install (B));
	  sinkAppA.Start (Seconds (0));
	  // the following means the receiver will run 1 min longer than the sender app.
	  //sinkAppA.Stop (Seconds (runtime-1.0*i));
	  sinkAppA.Stop (Seconds (runtime));
    }

  

  // create a sender on A
  ObjectFactory factory;
  factory.SetTypeId ("ns3::BulkSendApplication");
  factory.Set ("Protocol", StringValue ("ns3::TcpSocketFactory"));
  factory.Set ("MaxBytes", UintegerValue (maxBytes));
  factory.Set ("SendSize", UintegerValue (tcpSegmentSize));
  factory.Set ("CCProtocol", StringValue (TCP_PROTOCOL));

  Ptr<Object> bulkSendAppObj ;
  Ptr<Application> bulkSendApp ;






  for(int i=0;i<(flowNum+1)/2;i++){


	  Address sinkAddr(InetSocketAddress(B->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), Bport+2*i));
	  factory.Set ("Remote", AddressValue (sinkAddr));
	  bulkSendAppObj = factory.Create();
	  bulkSendApp = bulkSendAppObj -> GetObject<Application>();


	  bulkSendApp->SetStartTime(Seconds(0.001*i));
	  bulkSendApp->SetStopTime(Seconds(runtime));

	  A1->AddApplication(bulkSendApp);
  }



  ObjectFactory attackFactory;
  attackFactory.SetTypeId ("ns3::BulkSendApplication");
  attackFactory.Set ("Protocol", StringValue ("ns3::TcpSocketFactory"));
  attackFactory.Set ("MaxBytes", UintegerValue (maxBytes));
  attackFactory.Set ("SendSize", UintegerValue (tcpSegmentSize));
  attackFactory.Set ("CCProtocol", StringValue (attacker_PROTOCOL));

  for(int i=0;i<flowNum/2;i++){//flowNum/2
	  Address sinkAddr(InetSocketAddress(B->GetObject<Ipv4>()->GetAddress(1,0).GetLocal(), Bport+2*i+1));
	  attackFactory.Set ("Remote", AddressValue (sinkAddr));
	  bulkSendAppObj = attackFactory.Create();
	  bulkSendApp = bulkSendAppObj -> GetObject<Application>();

	  bulkSendApp->SetStartTime(Seconds(0.001*i+attackTime));
	  bulkSendApp->SetStopTime(Seconds(atteckerExit));

	  A1->AddApplication(bulkSendApp);
  }


 
    // Set up tracing
  iftrace = false;
  if (false && iftrace){
  AsciiTraceHelper ascii;
  //std::string tfname = fileNameRoot + "Copa.tr";
  std::string tfname = "Copa.tr";
  AR1.EnableAsciiAll (ascii.CreateFileStream (tfname));

  //
  }
  // Setup tracing for cwnd
  //double postfix=1/delta;
  if (iftrace){

	  Simulator::Schedule(Seconds(0.0),&TraceQueue,postfix);
	  Simulator::Schedule(Seconds(0.01),&TraceCwnd,flowNum);
	  AsciiTraceHelper ascii;
	  std::stringstream stemp1;
	  stemp1<<fileNameRoot<<"PDCC.Thr";
	  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (stemp1.str());
	  Simulator::Schedule(Seconds(0.02),&TraceThr,flowNum,0,0,&sinkAppA,stream);
  }

  Simulator::Stop (Seconds (runtime+1));
  Simulator::Run ();
  if (iftrace){
	  //Simulator::Schedule(Seconds(0.0),&TraceCwnd);       // this Time cannot be 0.0
	  AsciiTraceHelper ascii;
	  std::stringstream stemp;
	  stemp<<fileNameRoot<<flowNum<<".receive";
	  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (stemp.str());
	  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkAppA.Get (0));
	  for(int i=0;i<flowNum;i++){
		  sink1 = DynamicCast<PacketSink> (sinkAppA.Get (i));
		  *stream->GetStream () << "Total Bytes Received from A: " <<i<<" "<< sink1->GetTotalRx () << std::endl;
	  }
  }



  return 0;
}
