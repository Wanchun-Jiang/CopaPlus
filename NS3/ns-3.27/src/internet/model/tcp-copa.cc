/*
 * tcp-copa.cc
 *
 *  Created on: 12 23 2018
 *      Author: root
 */

#include "tcp-copa.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/log.h"
#include <cmath>
#include <fftw3.h>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpCopa");
NS_OBJECT_ENSURE_REGISTERED (TcpCopa);

TypeId
TcpCopa::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpCopa")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpCopa> ()
    .SetGroupName ("Internet")
    .AddAttribute ("delta", "the target packt queue in the buffer",
                   DoubleValue (2),
                   MakeDoubleAccessor (&TcpCopa::m_defaultDelta),
                   MakeDoubleChecker<double> ())
   .AddAttribute ("enableComp", "enable the competitive mode",
		   	   	      BooleanValue (false),
					  MakeBooleanAccessor (&TcpCopa::enableComp),
					  MakeBooleanChecker ())
   .AddAttribute ("iscopaModify", "the mode of adjusting cwnd copa or copaModify",
		   	   	      BooleanValue (false),
					  MakeBooleanAccessor (&TcpCopa::iscopaModify),
					  MakeBooleanChecker ())
   .AddAttribute ("cpVersion", "use the strawman version competitive mode modeSwitch: 2-strawman 0-origin 1-final",
		   	   	      UintegerValue (1),
					  MakeUintegerAccessor (&TcpCopa::cpVersion),
					  MakeUintegerChecker<uint32_t> ())
   .AddAttribute ("Qtarget", "the mode of adjusting cwnd copa or copaModify",
					  TimeValue (Seconds(0.0024)),
					  MakeTimeAccessor (&TcpCopa::Qtarget),
					  MakeTimeChecker ())
   .AddAttribute ("assitPra", "the asist pramete, time window of RTTmin",
					 DoubleValue (10),
					 MakeDoubleAccessor (&TcpCopa::assitPra),
					 MakeDoubleChecker<double> ())
   .AddAttribute ("gama", "gama for Copa+",
					 DoubleValue (0.85),
					 MakeDoubleAccessor (&TcpCopa::m_gama),
					 MakeDoubleChecker<double> ())
  ;
  return tid;
}

TcpCopa::TcpCopa (void)
  : TcpNewReno (),
    m_delta (2),
	m_v(1),
	m_srtt (Time::Max ()),
    m_cntRtt (0),
    m_doingCopaNow (true),
    m_begSndNxt (0),
	m_RTTmin (Time::Max ()),
	recordTimeMin (Time (0.0)),
	m_RTTstanding (Time::Max ()),
    m_Tmin (Time::Max ()),
    m_Tmax (Time::Min ()),
	m_oldRTTstanding(Time::Max ()),
	recordTimeStanding (Time (0.0)),
	oldCwndpkts(1.0),
	newCwndpkts(1.0),
	oldAckTime(0),
	oldAckSeq(0),
	pra(1),
	oldRtt1(Time::Max ()),
	oldRtt2(Time::Max ()),
	oldDirect(true),
	newDirect(true),
	times(0),
	isFirstTime(true),
	cpModeInitial(true),
	iniProc(true),
	m_Wmax(0),
	rttInc(0),
	cwndInc(0),
	cwndTot(0),
	cwndBeforeCo(0),
	cpCnt(0),
	thrTemp(0),
	thrMax(0),
	tZero(Time (0.0)),
	tLoss(Time (0.0)),
	tDeltaUpdate(Time (0.0)),
	m_itv (Time (0.0)),
	lastCheck(Time (0.0)),
	rttAvg(0.0),
	m_gama (0.85),
	Qtarget(Time(Seconds(0.0024))),
	isSlowStart(true),
	m_RTTmax (Time (0.0)),
	globalRttMax(Time (0.0)),
	tempEmpty(false),
	compEmpty(false),
	defaultMode(true),
	RTTCountForEmpty(0),
 	RTTCountForMax(0),
	tempMax(Time (0.0)),
	nearEmpty(true),
	assitPra(0),
	Epra(2),
	oldEpra(1),
	v0(1),
	RTTCountFormin(0),
	NEpra(1),
	RTTCount(0),
   isBiTheta(false),
   ssTheta(1),
   sstarget(5),
   Smax(8),
   recordTimeEmpty (Time (0.0)),
   directChange(false),
   RTTforCountDirectChange(0),
   ForEmpty(2),
   oldCwndMid(1.0),
   m_RTTq(Time::Max ()),
   ackCount(0),
   intervalPoint(0.0),
   srttPoint(0.0),
   simiLast(1.0),
   queMin(),
   queStd(),
   queTmin(),
   queTmax()
{
  NS_LOG_FUNCTION (this);
}

TcpCopa::TcpCopa (const TcpCopa& sock)
  : TcpNewReno (sock),
	m_delta (sock.m_delta),
	m_v (sock.m_v),
	m_srtt (sock.m_srtt),
    m_cntRtt (sock.m_cntRtt),
	m_doingCopaNow (true),
    m_begSndNxt (0),
	m_RTTmin (Time::Max ()),
	recordTimeMin (Time (0.0)),
	m_RTTstanding (Time::Max ()),
    m_Tmin (Time::Max ()),
    m_Tmax (Time::Min ()),
	m_oldRTTstanding(Time::Max ()),
	recordTimeStanding (Time (0.0)),
	oldCwndpkts(1.0),
	newCwndpkts(1.0),
	oldAckTime(0),
	oldAckSeq(0),
	pra(1),
	oldRtt1(Time::Max ()),
	oldRtt2(Time::Max ()),
	oldDirect(true),
	newDirect(true),
	times(0),
	isFirstTime(true),
	cpModeInitial(true),
	iniProc(true),
	m_Wmax(0),
	rttInc(0),
	cwndInc(0),
	cwndTot(0),
	cwndBeforeCo(0),
	cpCnt(0),
	thrTemp(0),
	thrMax(0),
	tZero(Time(0.0)),
	tLoss(Time (0.0)),
	tDeltaUpdate(Time(0.0)),
	m_itv(Time (0.0)),
	lastCheck(Time (0.0)),
	rttAvg(0.0),
	m_gama (0.85),
	Qtarget(Time(Seconds(0.0024))),
	isSlowStart(true),
	m_RTTmax(Time (0.0)),
	globalRttMax(Time (0.0)),
	tempEmpty(false),
	compEmpty(false),
	defaultMode(true),
	RTTCountForEmpty(0),
 	RTTCountForMax(0),
	tempMax(Time (0.0)),
	nearEmpty(true),
	assitPra(0),
	Epra(4),
	oldEpra(4),
	v0(1),
	RTTCountFormin(0),
	NEpra(1),
	RTTCount(0),
   isBiTheta(false),
   ssTheta(1),
   sstarget(5),
   Smax(4),
   recordTimeEmpty (Time (0.0)),
   directChange(false),
   RTTforCountDirectChange(0),
   ForEmpty(2),
   oldCwndMid(1.0),
   m_RTTq(Time::Max ()),
   ackCount(0),
   intervalPoint(0.0),
   srttPoint(0.0),
   simiLast(1.0),
   queMin(),
   queStd(),
   queTmin(),
   queTmax()
{
  NS_LOG_FUNCTION (this);
}

TcpCopa::~TcpCopa (void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps>
TcpCopa::Fork (void)
{
  return CopyObject<TcpCopa> (this);
}

void TcpCopa::Send(Ptr<TcpSocketBase> tsb, Ptr<TcpSocketState> tcb,
                  SequenceNumber32 seq, bool isRetrans) {

  NS_LOG_FUNCTION(this);



  // If retransmission, start sequence (PktsAcked() finds end of sequence).
  if (isRetrans) {
    NS_LOG_LOGIC(this << "  Starting retrans sequence: " << seq);
  }else {

	  cwndHistory.insert(std::pair<SequenceNumber32,uint32_t>(tcb->m_nextTxSequence,tcb->m_cWnd));
	  ackTimeHistory.insert(std::pair<SequenceNumber32,Time>(tcb->m_nextTxSequence,Simulator::Now ()));
	  ackSeqHistory.insert(std::pair<SequenceNumber32,SequenceNumber32>(tcb->m_nextTxSequence,tcb->m_lastAckedSeq));
  }
}

void
TcpCopa::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                     const Time& rtt)
{
	NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);


	if (rtt.IsZero ())
	{
	return;
	}
	int64_t nowToInt = Simulator::Now ().GetInteger();
	//int64_t recordTimeMinToInt = recordTimeMin.GetInteger();
    
    //update RTTmin
	Time tempRTT = this->GetrttInst();
	while(!queMin.empty() && queMin.back().second > tempRTT)queMin.pop_back();
	queMin.push_back(std::make_pair(nowToInt,tempRTT));
	while(queMin.size()>1 && (queMin.front().first + std::max(Seconds (assitPra).GetInteger(),int64_t(100*m_RTTmin.GetInteger()))) < nowToInt)queMin.pop_front();// dynamic RTTmin time window by lhy 2020.10.15
	m_RTTmin=queMin.front().second;
    
    while(!queStd.empty() && queStd.back().second > tempRTT)queStd.pop_back();
	queStd.push_back(std::make_pair(nowToInt,tempRTT));
	while(queStd.size()>1 && (queStd.front().first + rtt.GetInteger()/2) < nowToInt)queStd.pop_front();// dynamic RTTmin time window by lhy 2020.10.15
	m_RTTstanding=queStd.front().second;
    
    while(!queTmin.empty() && queTmin.back().second > tempRTT)queTmin.pop_back();
	queTmin.push_back(std::make_pair(nowToInt,tempRTT));
	while(queTmin.size()>1 && (queTmin.front().first + rtt.GetInteger()*4) < nowToInt)queTmin.pop_front();// dynamic RTTmin time window by lhy 2020.10.15
	m_Tmin=queTmin.front().second;
    
    while(!queTmax.empty() && queTmax.back().second < tempRTT)queTmax.pop_back();
	queTmax.push_back(std::make_pair(nowToInt,tempRTT));
	while(queTmax.size()>1 && (queTmax.front().first + rtt.GetInteger()*4) < nowToInt)queTmax.pop_front();// dynamic RTTmin time window by lhy 2020.10.15
	m_Tmax=queTmax.front().second;
  
	globalRttMax = std::max(globalRttMax,m_Tmax);

   compEmpty=(m_Tmin.GetSeconds() - m_RTTmin.GetSeconds() <=0.1*(m_Tmax.GetSeconds()-m_RTTmin.GetSeconds()));

	if(iscopaModify){
		PktsAckedACopa(tcb,segmentsAcked,rtt);

	}
	else {
		//update RTTmin
		/* if(nowToInt > recordTimeMinToInt + Seconds (10.0).GetInteger()) {
			m_RTTmin = Time::Max ();
		}
		if (this->GetrttInst() < m_RTTmin) {
			recordTimeMin = Simulator::Now ();

		}
		if(this->GetrttInst() == m_RTTmin){
			recordTimeMin = Simulator::Now ();
		}
		m_RTTmin = std::min (m_RTTmin, this->GetrttInst()); */

		// initial RTTmax , m_oldRTTstanding
		if(m_RTTmax==Time (0.0)) m_RTTmax=this->GetrttInst();
		if(m_oldRTTstanding == Time::Max ()) m_oldRTTstanding = this->GetrttInst();

		//update tempmax
		tempMax = std::max (tempMax, this->GetrttInst());

		//update m_RTTstanding
		/* int64_t recordTimeStandingToInt = recordTimeStanding.GetInteger();
		if(nowToInt > recordTimeStandingToInt + rtt.GetInteger()/2){

			m_RTTstanding = Time::Max ();
			m_RTTstanding = std::min (m_RTTstanding, this->GetrttInst());
			oldCwndMid=tcb->m_cWnd;
			m_oldRTTstanding = m_RTTstanding;
			//m_RTTstanding = Time::Max ();
			recordTimeStanding = Simulator::Now ();
			srttPoint = Simulator::Now ().GetSeconds();
		}else{
			srttPoint=0;
			m_RTTstanding = std::min (m_RTTstanding, this->GetrttInst());
		} */

		//double Qd = this->GetrttInst().GetSeconds() - m_RTTmin.GetSeconds();
		double Qd = m_RTTstanding.GetSeconds() - m_RTTmin.GetSeconds();
		Qd = std::max(0.0,Qd);

		//compute tempEmpty
		if(!isSlowStart){
			//if(Qd <=0.1*(m_RTTmax.GetSeconds()-m_RTTmin.GetSeconds())) tempEmpty = true;
						//compEmpty=(m_Tmin.GetSeconds() - m_RTTmin.GetSeconds() <=0.1*(m_Tmax.GetSeconds()-m_RTTmin.GetSeconds()));
            if(compEmpty) tempEmpty = true;
		}
		nearEmpty=tempEmpty;

		double currentRate = tcb->m_cWnd/m_RTTstanding.GetSeconds();
		double targetrate = m_delta*tcb->m_segmentSize/Qd;

		if(currentRate<=targetrate){
		  if(oldDirect==false) {
			  //m_v=1;
			  directChange=true;
		  }
		}
		if(currentRate>targetrate){
		  isSlowStart=false;
		  if(oldDirect==true) {
			  //m_v=1;
			  directChange=true;
		  }
		}


		if(!isSlowStart){
				if(enableComp){
					defaultMode=compEmpty;
				}
				else defaultMode=true;
			}

	   intervalPoint=0;
		if (tcb->m_lastAckedSeq >= m_begSndNxt)
		{ // A Copa cycle has finished, we do Copa cwnd adjustment every RTT.
			intervalPoint=Simulator::Now ().GetSeconds();
			RTTCountForMax++;

			if(!isSlowStart) {
				RTTCount++;
			}

			for(int i=0;i<3;i++){
				RTTHistory[i]=RTTHistory[i+1];
			}
			RTTHistory[3]=tempMax.GetSeconds();
			tempMax=Time (0.0);

			double rttMax=0;
			if(RTTCountForMax>=4){
					for(int i=0;i<4;i++){
						rttMax=std::max(rttMax, RTTHistory[i]);
				}
			}
			m_RTTmax=Time(Seconds(rttMax));

			for(int i=0;i<4;i++){
				emptyHistory[i]=emptyHistory[i+1];
			}
			emptyHistory[4]=tempEmpty;
			tempEmpty=false;

			newCwndpkts = tcb->m_cWnd/(double)tcb->m_segmentSize;
			if(newCwndpkts > oldCwndpkts ) newDirect = true; //|| newDirect == true
			if(newCwndpkts <= oldCwndpkts ) newDirect = false; //|| newDirect == false

			if(!isSlowStart){
				if (newDirect == oldDirect){//&&!directChange
				  times ++;
				  if(isFirstTime){
					  if(times > 3){
							if(m_v*2 < (double)tcb->m_cWnd/(tcb->m_segmentSize*m_delta)){
								 m_v = m_v*2;
								  }

						  isFirstTime = false;
						  }
					  }
				  else {

						if(m_v*2 < (double)tcb->m_cWnd/(tcb->m_segmentSize*m_delta)){
							 m_v = m_v*2;
							  }
					  }
				}
				else {
				  directChange=false;
				  //RTTCountForEmpty++;
				  isFirstTime =true;
				  times=0;
				  m_v=1;
				}
			}

			nearEmpty=false;
			// for(int i=0;i<5;i++){
			// 	 if(emptyHistory[i]){
			// 		 nearEmpty=true;
			// 		 break;
			// 	 }
			// }
			
			NS_LOG_LOGIC ("A Copa cycle has finished, we adjust cwnd once per RTT.");
			oldRtt1=m_RTTstanding;

			m_begSndNxt = tcb->m_nextTxSequence;
			oldDirect = newDirect;
			oldCwndpkts = newCwndpkts;

			if(!defaultMode){
					m_delta=m_delta+1;
				}

		}

		if(defaultMode){
					m_delta=m_defaultDelta;
				}

		//NS_LOG_INFO(this<<" lhyDebug--compForCopa: now "<<nowToInt/1000000000.0 << " isSlowStart " << isSlowStart << " compEmpty " << compEmpty << " enableComp " << enableComp);
	}
	uint32_t inflight = tcb->m_nextTxSequence.Get().GetValue() - tcb->m_lastAckedSeq.GetValue();
	double temp = tcb->m_segmentSize/(double)tcb->m_cWnd;


	/************** Loss detect ******************/
	//bool tempFlag = 1;
  // if (!defaultMode && tcb->m_congState == TcpSocketState::CA_LOSS)
  // 	{
  // 		std::cout<< this <<" lhyDebug--Loss-reset "<< Simulator::Now() << std::endl;
  // 		defaultMode=true;
		// 	m_Wmax=0;
		// 	tZero=Time(0.0);
		// 	m_itv=Time(0.0);
		// 	tempFlag = 0;
		// 	//thrMax=0;
		// 	if(cpVersion==1) tcb->m_cWnd *= m_gama;
  // 	}
  // else if (Simulator::Now()-tZero>m_RTTstanding && (/*tcb->m_congState == TcpSocketState::CA_LOSS || */tcb->m_congState == TcpSocketState::CA_RECOVERY))
	 //  {
	 //  	std::cout<< this <<" lhyDebug--Loss-react "<< Simulator::Now() << " prev " << tZero << std::endl;
  //     if(!defaultMode)
  //     {
  //   	  if(cpVersion==0 || cpVersion==2) 
  //   	  {
  //   	  	m_delta=std::max(m_delta/2,2.0);
  //   	  	if(m_delta==2.0) defaultMode=true;
  //   	  	m_gama=1;
  //   	  }
  //   	  else
  //   	  {
  //   	  	m_Wmax = tcb->m_cWnd;

  //   	  	if(tZero > Time(0.0))
  //   	  	{
  //   	  		if(m_itv == 0) m_itv = Simulator::Now()-tZero;
  //   	  		else m_itv = m_itv/2 + (Simulator::Now()-tZero)/2;
  //   	  	}
  //   	  	tcb->m_cWnd *= m_gama;
  //   	  	tempFlag = 0;
  //   	  	rttAvg=rttAvg/cpCnt;
  //   	  	if(cpCnt) m_alpha = calculateAlpha(tcb);
  //   	  	else m_alpha = 1.0;
  //   	  	rttAvg=0;
  //   	  	cpCnt=0;
  //   	  }
  //     	tZero = Simulator::Now();
  //     }
	 //    else
	 //    {
	 //      m_delta=m_defaultDelta;
	 //    }
		//   NS_LOG_LOGIC(this<<" delta "<<m_delta);
	 //  }

	//IncreaseWindow_perAck(tcb,segmentsAcked);


	NS_LOG_INFO(this<<" rttI "<<this->GetrttInst()<<" Stand "<<m_RTTstanding
		  <<" Old "<<m_oldRTTstanding<<" RTTmin "<<m_RTTmin
		  <<" Qd "<<(m_RTTstanding- m_RTTmin).GetSeconds()<<" srttPoint "<<srttPoint/1000000000.0
		  <<" now "<<nowToInt/1000000000.0<<" timePoint "<< intervalPoint
		  <<" cwnd "<<tcb->m_cWnd/1448.0<<" CRate "<<(12000/temp)/m_RTTstanding.GetSeconds()
		  <<" Trate "<<m_delta*12000/(m_RTTstanding- m_RTTmin).GetSeconds()<<" pRate "<<tcb->GetPacingRate()
		  <<" cwnd(t-rtt) "<<cwndHistory.find(tcb->m_lastAckedSeq)->second
		  <<" inflight "<<inflight/1448.0<<" segAcked "<<segmentsAcked
		  <<" delta "<<m_delta<<" v "<<m_v
		  <<" lsAck "<<tcb->m_lastAckedSeq.GetValue()/1448.0
		  <<" tempMax "<<tempMax.GetSeconds()<<" tempEmpty "<<tempEmpty
		  <<" RTTHistory[3] "<<RTTHistory[3]<<" emptyHistory[4] "<<emptyHistory[4]
		  <<" m_RTTmax "<<m_RTTmax.GetSeconds()<<" nearEmpty "<<nearEmpty
		  <<" pra "<<pra<<" Epra "<<Epra
		  <<" NEpra "<<NEpra<<" RTTCountForEmpty "<<RTTCountForEmpty<<" RTTCount "<<RTTCount<<" newDirect "<<newDirect<<" times "<<times
		  <<" m_RTTq "<<m_Tmin<<" temp "<<temp<<" RTTforCountDC "<<RTTforCountDirectChange<<" srtt "<<rtt
		  <<" forempty "<<ForEmpty<< " cpMode "<<defaultMode << " alpha " <<m_alpha << " compEmpty--Tmax--Tmin " 
		  << compEmpty << " " << m_Tmax.GetSeconds() << " " << m_Tmin.GetSeconds());
}

bool TcpCopa::modeSwitch(Ptr<TcpSocketState> tcb, const Time& rtt)
{
	
	

	int n=rttArray.size();

	thrMax=std::max(thrMax,(uint32_t)(1.0*thrTemp*tcb->m_segmentSize/(Simulator::Now().GetSeconds()-lastCheck.GetSeconds())));

	if (n==0)
	{
		lastCheck = Simulator::Now();
		return 1;
	}

	double fs=n/(20 * m_RTTmin.GetSeconds());
	fftw_complex *outa,*ina;
	outa = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
	ina = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
	double *rtt_amp = new double [n];
	double simi;
	double tempRTTavg=0,tempCwndavg=0;
	
	for(int i=0;i<n;i++)
	{
		ina[i][0]=rttArray[i];
        ina[i][1]=0;
        tempRTTavg+=rttArray[i];
	} 
	fftw_plan p=fftw_plan_dft_1d(n,ina,outa,FFTW_FORWARD,FFTW_ESTIMATE);
	fftw_execute(p);
	fftw_destroy_plan(p);
	double temp,qst=-1;
	
	for(int i=1;i<n/2;i++)
	{
		temp=sqrt(outa[i][0]*outa[i][0]+outa[i][1]*outa[i][1]);
		if(i*fs/n>1.5 && qst<temp)
		{
			qst=temp;
		}
	}
	
	//double P_ave=sqrt(outa[(int)*fm][0]*outa[(int)*fm][0]+outa[(int)*fm][1]*outa[(int)*fm][1]);
	qst=sqrt(outa[0][0]*outa[0][0]+outa[0][1]*outa[0][1]);
	rtt_amp[0]=1;
	for(int i=1;i<n/2;i++)
	{
		rtt_amp[i]=sqrt(outa[i][0]*outa[i][0]+outa[i][1]*outa[i][1])*2/qst;
	}
	
	for(int i=0;i<n;i++)
	{
		ina[i][0]=cwndArray[i];
        ina[i][1]=0;
        tempCwndavg+=cwndArray[i];
	}
	fftw_plan pp=fftw_plan_dft_1d(n,ina,outa,FFTW_FORWARD,FFTW_ESTIMATE);
	fftw_execute(pp);
	fftw_destroy_plan(pp);
	for(int i=1;i<n/2;i++)
	{
		temp=sqrt(outa[i][0]*outa[i][0]+outa[i][1]*outa[i][1]);
		if(i*fs/n>1.5 && qst<temp)
		{
			qst=temp;
		}
	}
	qst=sqrt(outa[0][0]*outa[0][0]+outa[0][1]*outa[0][1]);
	for(int i=1;i<n/2;i++)
	{
		temp=sqrt(outa[i][0]*outa[i][0]+outa[i][1]*outa[i][1])*2/qst;
		simi+=fabs(temp-rtt_amp[i]);
	}
	
	
	fftw_free(outa);
	fftw_free(ina);
	delete rtt_amp;
	double simiD=abs(double(1.0*rttInc)-double(1.0*cwndInc))/n;

	rttArray.clear();
	cwndArray.clear();
	//thrTemp/=1.0*Simulator::Now().GetMilliSeconds()-lastCheck.GetMilliSeconds();
	simi/=n;
	std::cout<< this <<" lhyDebug: "<<" prev at "<<lastCheck<< " Df "<<simi << " smoothDf " << simiLast <<" Dt "<< simiD << " rtt/cwnd Inc - n "<< rttInc<<" "<<cwndInc << " " << n <<" thrTemp "<<thrTemp<< " loss_itv " << m_itv << " loss_point " << tLoss << " Wmax " << m_Wmax/tcb->m_segmentSize <<" res "<< !(simi >= 0.02 || simiD >= 0.6) <<std::endl;
	if(!(simi >= 0.02 || simiD >= 0.6) && defaultMode)cwndBeforeCo=cwndTot/n; // modify the initial cwnd when entering competive mode by lhy 0622
	rttInc=0;
	cwndInc=0;
	
	//thrMax=std::max(thrMax,(uint32_t)(thrTemp*tcb->m_segmentSize/(1.0*Simulator::Now().GetMilliSeconds()-lastCheck.GetMilliSeconds())));
	//thrMax=std::max(thrMax,(uint32_t)(1.0*tempCwndavg/tempRTTavg));
	thrTemp=0;
	cwndTot=0;

	lastCheck = Simulator::Now();

	return !(simi >= 0.02 || simiD >= 0.6);
	// if((simi > simiLast*5 || simiD >= 0.6) && !iniProc)
	// {
	// 	return false;
	// }
	// else
	// {
	// 	if(iniProc) iniProc = false;
	// 	else if(simiLast == 1.0) simiLast = simi;
 //    else simiLast = simiLast/2 + simi/2;
	// 	return true;
	// }
}

void
TcpCopa::opModeForCopaplus(Ptr<TcpSocketState> tcb, const Time& rtt)
{
	if(!isSlowStart){
			if(enableComp){
				//if(cpVersion==2)defaultMode = (nearEmpty && !RTTforCountDirectChange) ; 
				if(cpVersion==2)defaultMode = (compEmpty); //|| (RTTforCountDirectChange>0)) ; 
				else if(cpVersion==0)defaultMode = compEmpty;
				else
				{ 
					rttAvg+=rtt.GetMilliSeconds();
					cpCnt++;

					//if(!defaultMode && m_itv > 0 && m_itv/2 + (Simulator::Now()-tZero)/2 >= 2*m_itv && tcb->m_cWnd > m_Wmax)
					if(!defaultMode && m_itv > 0 && m_itv/2 + (Simulator::Now()-tLoss)/2 >= 1.5*m_itv && tcb->m_cWnd > m_Wmax)
					{
						defaultMode=true;
						m_Wmax=0;
						//tcb->m_cWnd = (thrMax*m_RTTmin.GetSeconds());

						iniProc = true;

						lastCheck=Time(0.0);						
						tZero=Time(0.0);
						tLoss=Time(0.0);
						m_itv=Time(0.0);

					}
					if(defaultMode && Simulator::Now()-lastCheck < 20 * m_RTTmin) 
					{
						rttArray.push_back(rtt.GetSeconds());
					  cwndArray.push_back(tcb->m_cWnd);
					  if(rtt>old_rtt)rttInc++;
				   	if(tcb->m_cWnd>old_cwnd)cwndInc++;
				   	old_rtt=rtt;
				   	old_cwnd=tcb->m_cWnd;
				   	cwndTot+=tcb->m_cWnd;
				   	thrTemp++;
					}
					else
					{ 
						bool modeTemp=defaultMode;
						defaultMode&=modeSwitch(tcb, rtt);
						if(modeTemp^defaultMode) cpModeInitial=true;
					}
					
				}
			}
			else defaultMode=true;


			if(!defaultMode){
				if(cpVersion==0 || cpVersion==2) 
				{
					if(Simulator::Now()-tDeltaUpdate>m_RTTstanding)
					{
						if(!RTTdirect && cpVersion==2) m_delta=std::max(m_delta/2,2.0);
						else m_delta=m_delta+1;
						tDeltaUpdate = Simulator::Now();
					}
					if(m_delta==m_defaultDelta) defaultMode=true;
				}
				else if(cpModeInitial) 
				{
					tcb->m_cWnd = std::max((double)tcb->m_cWnd, (thrMax*globalRttMax.GetSeconds()));
					m_Wmax = tcb->m_cWnd;
					//tcb->m_cWnd *= m_gama;
					//m_itv = MilliSeconds(std::pow(m_Wmax/tcb->m_segmentSize*(1-(717.0/1024.0))/(410.0/1024.0),1.0/3.0) * 1000)/2;

					thrMax = 0;
					globalRttMax = Time(0.0);
					//tcb->m_cWnd = cwndBeforeCo;
					m_alpha = 1.0;
					cpModeInitial = 0;
				}
			}else{
				m_delta=m_defaultDelta;
			}
		}
}

void
TcpCopa::PktsAckedACopa (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                     const Time& rtt)
{
	NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);

	if (rtt.IsZero ())
	{
		return;
	}

	opModeForCopaplus(tcb,rtt);

	//int64_t nowToInt = Simulator::Now ().GetInteger();
	//int64_t recordTimeMinToInt = recordTimeMin.GetInteger();

	/* if(nowToInt > recordTimeMinToInt + Seconds (10.0).GetInteger()) {
		m_RTTmin = Time::Max ();
	}
	if (this->GetrttInst() < m_RTTmin) {
		RTTCountFormin=0;
		recordTimeMin = Simulator::Now ();

	}
	if(this->GetrttInst() == m_RTTmin){
		RTTCountFormin++;
		recordTimeMin = Simulator::Now ();
	}
	m_RTTmin = std::min (m_RTTmin, this->GetrttInst()); */

	if(m_RTTmax==Time (0.0)) m_RTTmax=this->GetrttInst();
	if(m_oldRTTstanding == Time::Max ()) m_oldRTTstanding = this->GetrttInst();


	uint32_t Cwmdthres=3000;

	//int64_t recordTimeStandingToInt = recordTimeStanding.GetInteger();
	//if(nowToInt > recordTimeStandingToInt + rtt.GetInteger()/2){
	if(m_oldRTTstanding != m_RTTstanding){
	   //m_RTTstanding = Time::Max ();
	   //m_RTTstanding = std::min (m_RTTstanding, this->GetrttInst());
		if(tcb->m_cWnd<=Cwmdthres*tcb->m_segmentSize){//true||
			if(m_RTTstanding>m_oldRTTstanding){
				if(RTTdirect==false){
					if(tempEmpty){
						RTTCountForEmpty++;
					}
					RTTforCountDirectChange++;
				}
				RTTdirect=true;
			}else{
			  if(RTTdirect==true){
					if(tempEmpty){
						RTTCountForEmpty++;
					}
					RTTforCountDirectChange++;
					m_RTTmax=tempMax;
					tempMax=Time::Min ();
			  }
			  RTTdirect=false;
			}
		}
		oldCwndMid=tcb->m_cWnd;
		m_oldRTTstanding = m_RTTstanding;
		//m_RTTstanding = Time::Max ();
		recordTimeStanding = Simulator::Now ();
		srttPoint = Simulator::Now ().GetSeconds();
	}
    /* else{
	  m_RTTstanding = std::min (m_RTTstanding, this->GetrttInst());
	}
 */
	if(ackCount>(Cwmdthres/2)){
		m_RTTq = Time::Max ();
		m_RTTq = std::min (m_RTTq, rtt);
		if(tcb->m_cWnd>Cwmdthres*tcb->m_segmentSize){//false&&
			if(m_RTTq>oldRtt2){
				if(RTTdirect==false){
					if(tempEmpty){
						RTTCountForEmpty++;
					}
					RTTforCountDirectChange++;
				}
				RTTdirect=true;
			}else{
			  if(RTTdirect==true){
					if(tempEmpty) {
						RTTCountForEmpty++;
					}
					RTTforCountDirectChange++;
					m_RTTmax=tempMax;
					tempMax=Time::Min ();
			  }
			  RTTdirect=false;
			}
		}
		ackCount=0;
		oldRtt2 = m_RTTq;
	}else{
		ackCount++;
		m_RTTq = std::min (m_RTTq, rtt);
	}

	tempMax = std::max (tempMax, this->GetrttInst());

	//double Qd = this->GetrttInst().GetSeconds() - m_RTTmin.GetSeconds();
	double Qd = m_RTTstanding.GetSeconds() - m_RTTmin.GetSeconds();
	Qd = std::max(0.0,Qd);

	//int mEmp=0.01;

	tempMax = std::max (tempMax, this->GetrttInst());
	if(!isSlowStart){
		if(tcb->m_cWnd>Cwmdthres*tcb->m_segmentSize){
			//if((this->GetrttInst()-m_RTTmin).GetSeconds() <=mEmp*(m_RTTmax-m_RTTmin).GetSeconds()) tempEmpty = true;
		}else{
			//if(Qd <=mEmp*(m_RTTmax.GetSeconds()-m_RTTmin.GetSeconds())) tempEmpty = true;
		}
		if(Qd <=0.01*(m_RTTmax.GetSeconds()-m_RTTmin.GetSeconds())) tempEmpty = true;
		//if(compEmpty) tempEmpty = true;
		//compEmpty = (m_Tmin.GetSeconds() - m_RTTmin.GetSeconds() <= 0.1*(m_Tmax.GetSeconds()-m_RTTmin.GetSeconds()));
	}


	nearEmpty=tempEmpty;
	if(RTTforCountDirectChange>=2&&RTTforCountDirectChange%2==0&&RTTCountForEmpty>=2&&RTTCountForEmpty%2==0){
		tempEmpty=false;
	}
	if((defaultMode || cpVersion ==0 || cpVersion == 2) && iscopaModify&&!isSlowStart){
		if(RTTforCountDirectChange>=2&&m_v==1){

			if(nearEmpty&&RTTCountForEmpty>=ForEmpty){//&& RTTCountForEmpty>=10,&&RTTCountForEmpty>=ForEmpty
				recordTimeEmpty=Simulator::Now ();
				Epra=pra;
				pra=0.75*pra;
//				NEpra=m_v*pra;
//				pra=0.75*pra;
				//pra=std::max(0.8*pra,1.0/(m_v*m_delta*temp*tcb->m_segmentSize));
				ForEmpty*=2;
				RTTCountForEmpty=0;
				RTTforCountDirectChange=0;
				//tempEmpty=false;
			}else if(!nearEmpty && RTTforCountDirectChange%2==0){
				ForEmpty=2;
				//pra=0.4*std::pow(((Simulator::Now ()-recordTimeEmpty)/m_RTTmin-std::pow(2*Epra,1/3.0)),3)+Epra;
				//double K=std::pow(20000*Epra,1.0/3.0);
				//double T_K=std::pow((RTTforCountDirectChange-K),3);
				//pra=0.00001*T_K+Epra;
				if(recordTimeEmpty==Time (0.0)) {
					recordTimeEmpty=Simulator::Now ();
				}
				double K=std::pow(2.5*Epra,1.0/3.0);
				double T_K=std::pow(((Simulator::Now ()-recordTimeEmpty).GetSeconds()-K),3);
				pra=0.1*T_K+Epra;
				pra=std::min ((double)tcb->m_cWnd/(m_v*tcb->m_segmentSize), pra);
//				double K=std::pow(0.5*NEpra,1.0/3.0);
//				double T_K=std::pow(((Simulator::Now ()-recordTimeEmpty).GetSeconds()-K),3);
//				pra=(0.5*T_K+NEpra)/m_v;
				RTTCountForEmpty=0;
			}
		}else if(m_v>1) {
		  //pra=1;
		  //Epra=2;
		}


	}
	  //pra=1;

	double currentRate = tcb->m_cWnd/m_RTTstanding.GetSeconds();
	double targetrate = m_delta*tcb->m_segmentSize/Qd;

	if(currentRate<=targetrate){
		if(oldDirect==false) {
		  //m_v=1;
		  directChange=true;
		}
	}
	if(currentRate>targetrate){
		isSlowStart=false;
		if(oldDirect==true) {
		  //m_v=1;
		  directChange=true;
		}
	}

	intervalPoint=0;
	if (tcb->m_lastAckedSeq >= m_begSndNxt)
	{ // A Copa cycle has finished, we do Copa cwnd adjustment every RTT.
	  //intervalPoint=Simulator::Now ().GetSeconds();
		intervalPoint=Simulator::Now ().GetSeconds();
		RTTCountForMax++;

		if(!isSlowStart) {
			RTTCount++;
		}
		newCwndpkts = tcb->m_cWnd/(double)tcb->m_segmentSize;
		if(newCwndpkts > oldCwndpkts) newDirect = true;
		if(newCwndpkts <= oldCwndpkts) newDirect = false;

		if(!isSlowStart){

			if (newDirect == oldDirect&&!directChange){
				times ++;
				if(isFirstTime){
					if(times > 3){

						if(m_v*pra*2 < (double)tcb->m_cWnd/(tcb->m_segmentSize*m_delta)){
						  m_v = m_v*2;
						}

						isFirstTime = false;
					}
				}
				else {
					if(m_v*pra*2 < (double)tcb->m_cWnd/(tcb->m_segmentSize*m_delta)){
						m_v = std::max(m_v,m_v*2);
					}
				}
			}
			else {
				directChange=false;
				isFirstTime =true;
				times=0;
				m_v=1;
			}
		}

		if(!defaultMode && cpVersion==1) m_v=pra=1;// deactiviate the v and theta by lhy 0621


		NS_LOG_LOGIC ("A Copa+ cycle has finished, we adjust cwnd once per RTT.");
		//oldRtt2=oldRtt1;
		oldRtt1=m_RTTstanding;

		m_begSndNxt = tcb->m_nextTxSequence;
		oldDirect = newDirect;
		oldCwndpkts = newCwndpkts;

	}

}

void
TcpCopa::EnableCopa (Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION (this << tcb);

  m_doingCopaNow = true;
  m_begSndNxt = tcb->m_nextTxSequence;
  m_cntRtt = 0;
}

void
TcpCopa::DisableCopa ()
{
  NS_LOG_FUNCTION (this);

  m_doingCopaNow = false;
}

void
TcpCopa::CongestionStateSet (Ptr<TcpSocketState> tcb,
                              const TcpSocketState::TcpCongState_t newState)
{
  NS_LOG_FUNCTION (this << tcb << newState);
  if (newState == TcpSocketState::CA_OPEN)
	{
  	EnableCopa (tcb);
	}
  else
  {
    std::cout<< this <<" lhyDebug--State "<< newState << " Cwnd " << tcb->m_cWnd/tcb->m_segmentSize << std::endl;
   //  if(newState == TcpSocketState::CA_LOSS)
   //  {
   //  	std::cout<< this <<" lhyDebug--Loss-reset "<< Simulator::Now() << std::endl;
  	// 	defaultMode=true;
			// m_Wmax=0;
			// tZero=Time(0.0);
			// m_itv=Time(0.0);
   //  }
	  // if (newState == TcpSocketState::CA_RECOVERY && Simulator::Now()-tZero>m_RTTstanding)
	  // {
   //    if(!defaultMode)
   //    {
   //  	  if(cpVersion==0 || cpVersion==2) m_delta=std::max(m_delta/2,2.0);
   //  	  else
   //  	  {
   //  	  	m_Wmax = tcb->m_cWnd;

   //  	  	if(tZero > Time(0.0))
   //  	  	{
   //  	  		if(m_itv == 0) m_itv = Simulator::Now()-tZero;
   //  	  		else m_itv = m_itv/2 + (Simulator::Now()-tZero)/2;
   //  	  	}
   //  	  	tcb->m_cWnd *= m_gama;
   //  	  	rttAvg=rttAvg/cpCnt;
   //  	  	m_alpha = calculateAlpha(tcb);
   //  	  	rttAvg=0;
   //  	  	cpCnt=0;
   //  	  }
   //    	tZero = Simulator::Now();
   //    }
	  //   else
	  //   {
	  //     m_delta=m_defaultDelta;
	  //   }
		 //  NS_LOG_LOGIC(this<<" delta "<<m_delta);
	  // }
  	//DisableCopa ();

	}
}

void
TcpCopa::ExitRecovery (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);


  //if(Simulator::Now().GetInteger()>=5000000000) m_delta=1;
  //m_delta=1.0/2*m_delta;

}


void
TcpCopa::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (!m_doingCopaNow)
    {
      // If Vegas is not on, we follow NewReno algorithm
      NS_LOG_LOGIC ("Copa is not turned on, we follow NewReno algorithm.");
      TcpNewReno::IncreaseWindow (tcb, segmentsAcked);
      return;
    }


    IncreaseWindow_perAck(tcb,segmentsAcked);

  return ;
}

double
TcpCopa::calculateAlpha(Ptr<const TcpSocketState> tcb)
{
	
	double C=410.0/1024.0,beta=717.0/1024.0;
	double bicK=std::pow(m_Wmax/tcb->m_segmentSize*(1-beta)/C,1.0/3.0) * 1000;

	//std::cout<< this << " updateAlpha: C;beta;bicK;rttAvg;Wmax: " <<  C << " " << beta << " " << bicK << " " << rttAvg << " " << m_Wmax <<endl;

	return m_Wmax/tcb->m_segmentSize*(3.0/20.0) * m_delta * rttAvg / bicK;
}

void
TcpCopa::copaModifyWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked){

	  //double Qd = this->GetrttInst().GetSeconds() - m_RTTmin.GetSeconds();
	  double Qd = m_RTTstanding.GetSeconds() - m_RTTmin.GetSeconds();
	  //double min = m_RTTmin.GetSeconds();
	  double stand = m_RTTstanding.GetSeconds(); stand+=0;


	  uint32_t oldseq=0; oldseq+=0;
	  uint32_t oldC=1;
	  if(cwndHistory.find(tcb->m_lastAckedSeq)->second<=0) {
		  oldC+=0;
	  }
	  else {
		  oldC = cwndHistory.find(tcb->m_lastAckedSeq)->second;
	  }

	  if(ackTimeHistory.find(tcb->m_lastAckedSeq)->second.GetSeconds()<=oldAckTime) {
	  }
	  else {
		  oldAckTime = ackTimeHistory.find(tcb->m_lastAckedSeq)->second.GetSeconds();
	  }

	  if(ackSeqHistory.find(tcb->m_lastAckedSeq)->second.GetValue()==0) {
		  oldseq = oldAckSeq;
	  }
	  else {
		  oldseq = ackSeqHistory.find(tcb->m_lastAckedSeq)->second.GetValue();
		  oldAckSeq = ackSeqHistory.find(tcb->m_lastAckedSeq)->second.GetValue();
	  }
	  //double currentRate = (tcb->m_lastAckedSeq.GetValue()-oldseq)/(Simulator::Now ().GetSeconds()-oldtime);
	  double currentRate = tcb->m_cWnd/m_RTTstanding.GetSeconds();

	  double targetrate = m_delta*tcb->m_segmentSize/Qd;
	  //double b=0.5;

	  //int64_t nowToInt = Simulator::Now ().GetInteger();
	  //m_v=1;
	  double temp = tcb->m_segmentSize*tcb->m_segmentSize/(double)tcb->m_cWnd;
	  //m_v=1;
	  if(defaultMode){
		  if(currentRate <= targetrate){ //currentRate <= targetrate*Qt/Qd

			  if(m_v>1){
				  double amplitude=std::max(1.0,m_v*m_delta*temp);
				  tcb->m_cWnd = std::min(2.0*tcb->m_cWnd,tcb->m_cWnd + amplitude);
			  }else{
				  double amplitude=std::max(1.0,m_v*pra*m_delta*temp);
				  tcb->m_cWnd = std::min(2.0*tcb->m_cWnd,tcb->m_cWnd + amplitude);
			  }

		  }
		  else {
			  if(m_v>1){
				  double amplitude=std::max(1.0,m_v*m_delta*temp);
				  	tcb->m_cWnd = std::max((double)tcb->m_segmentSize,(double)tcb->m_cWnd -amplitude);// m_v*pra*m_v*m_v*
			  }else{
				  double amplitude=std::max(1.0,m_v*pra*m_delta*temp);
				  tcb->m_cWnd = std::max((double)tcb->m_segmentSize,(double)tcb->m_cWnd -amplitude);
			  }

		  }
	  }else{
	  	if(cpVersion==0 || cpVersion==2)
	  	{
	  		temp*=m_v;
			  if(currentRate > targetrate)
				  tcb->m_cWnd = std::max((double)tcb->m_segmentSize,(double)tcb->m_cWnd - m_delta*temp);
			  else
				  tcb->m_cWnd = tcb->m_cWnd + m_delta*temp;
			}
			else
			{
				tcb->m_cWnd = tcb->m_cWnd + m_alpha*m_delta*temp;
			}
	  }
}

uint32_t
TcpCopa::GetSsThresh (Ptr<const TcpSocketState> tcb,
                       uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);

  

  if(tLoss > Time(0.0))
	{
    	if(m_itv == 0) m_itv = Simulator::Now()-tLoss;
    	else m_itv = m_itv/2 + (Simulator::Now()-tLoss)/2;
	}

	std::cout<< this <<" lhyDebug--Loss "<< Simulator::Now() << " prev " << tZero << " itvHis " << m_itv << " itvCur " << Simulator::Now()-tLoss << " Wmax " << m_Wmax/tcb->m_segmentSize << std::endl;

	tLoss = Simulator::Now();

  if (Simulator::Now()-tZero>m_RTTstanding)
	  {
      if(!defaultMode)
      {
    	  if(cpVersion==0 || cpVersion==2) 
    	  {
    	  	m_delta=std::max(m_delta/2,2.0);
    	  	m_gama=1;
    	  }
    	  else
    	  {
    	  	m_Wmax = tcb->m_cWnd;

    	  	// if(tZero > Time(0.0))
    	  	// {
    	  	// 	if(m_itv == 0) m_itv = Simulator::Now()-tZero;
    	  	// 	else m_itv = m_itv/2 + (Simulator::Now()-tZero)/2;
    	  	// }
    	  	//tcb->m_cWnd *= m_gama;
    	  	rttAvg=rttAvg/cpCnt;
    	  	if(cpCnt) m_alpha = calculateAlpha(tcb);
    	  	else m_alpha = 1.0;
    	  	rttAvg=0;
    	  	cpCnt=0;
    	  }
      	tZero = Simulator::Now();
      }
	    else
	    {
	      m_delta=m_defaultDelta;
	    }
		  NS_LOG_LOGIC(this<<" delta "<<m_delta);
	  }

  return std::max ((uint32_t)(m_gama * tcb->m_cWnd.Get()), 2 * tcb->m_segmentSize);
}

std::string
TcpCopa::GetName () const
{
  return "TcpCopa";
}

void
TcpCopa::IncreaseWindow_perAck (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  //if(Simulator::Now().GetInteger()>=5000000000) m_delta=1;

  //double Qd = this->GetrttInst().GetSeconds() - m_RTTmin.GetSeconds();
  double Qd = m_RTTstanding.GetSeconds() - m_RTTmin.GetSeconds();
  Qd = std::max(0.0,Qd);


  double currentRate = tcb->m_cWnd/m_RTTstanding.GetSeconds();
  double targetrate = m_delta*tcb->m_segmentSize/Qd;



  //m_v=1;
  double temp = m_delta*tcb->m_segmentSize/(double)tcb->m_cWnd;
  temp = temp*tcb->m_segmentSize;

  //m_v=1.0;
  if(isSlowStart){
	  //if (tcb->m_ssThresh>tcb->m_segmentSize*10000) tcb->m_ssThresh = tcb->m_segmentSize*32;

	  if(tcb->m_cWnd<tcb->m_ssThresh){
//		   if(tcb->m_cWnd<32*tcb->m_segmentSize){
//			   tcb->m_cWnd = std::max(2*tcb->m_segmentSize, (uint32_t)tcb->m_cWnd + tcb->m_segmentSize);
//		   }else{
//			   tcb->m_cWnd = tcb->m_cWnd +(double)32*tcb->m_segmentSize*tcb->m_segmentSize/tcb->m_cWnd;
//		   }
		   tcb->m_cWnd = std::max(2*tcb->m_segmentSize, (uint32_t)tcb->m_cWnd + tcb->m_segmentSize);

	  }else{

		   tcb->m_cWnd = tcb->m_cWnd +(double)tcb->m_segmentSize*tcb->m_segmentSize/tcb->m_cWnd;
		  tcb->m_ssThresh = 3.0/4.0 * tcb->m_cWnd;
	  }
  }
  else {
	  if(!iscopaModify){
		  if(currentRate > targetrate)
			  tcb->m_cWnd = std::max((double)tcb->m_segmentSize,(double)tcb->m_cWnd - m_v*temp);
		  else
			  tcb->m_cWnd = tcb->m_cWnd + m_v*temp;
	  }
	  else copaModifyWindow(tcb,segmentsAcked);
  }
  tcb->m_cWnd = std::max(2 * tcb->m_segmentSize, (uint32_t)tcb->m_cWnd.Get());
  double pacingRate = 0.0;
  //double tempSize = tcb->m_cWnd/(double)tcb->m_segmentSize;
  //tempSize = tempSize * (tcb->m_segmentSize + 52)*8;
  pacingRate = 2*tcb->m_cWnd*8/(m_RTTstanding.GetSeconds()*1000000);
  pacingRate += 0.0;
  //tcb->SetPacingRate(pacingRate);

  cwndHistory.erase(tcb->m_lastAckedSeq);
  ackTimeHistory.erase(tcb->m_lastAckedSeq);
  ackSeqHistory.erase(tcb->m_lastAckedSeq);
}

} // namespace ns3
