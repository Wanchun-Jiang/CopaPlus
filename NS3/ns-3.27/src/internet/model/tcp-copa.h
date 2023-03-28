/*
 * tcp-copa.h
 *
 *  Created on: 12 23 2018
 *      Author: root
 */


#ifndef TCPCOPA_H
#define TCPCOPA_H

#include "ns3/tcp-congestion-ops.h"
#include <queue>
#include <vector>
using namespace std;

namespace ns3 {

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of TCP Vegas
 *
 * TCP Vegas is a pure delay-based congestion control algorithm implementing a proactive
 * scheme that tries to prevent packet drops by maintaining a small backlog at the
 * bottleneck queue.
 *
 * Vegas continuously measures the actual throughput a connection achieves as shown in
 * Equation (1) and compares it with the expected throughput calculated in Equation (2).
 * The difference between these 2 sending rates in Equation (3) reflects the amount of
 * extra packets being queued at the bottleneck.
 *
 *              actual = cwnd / RTT        (1)
 *              expected = cwnd / BaseRTT  (2)
 *              diff = expected - actual   (3)
 *
 * To avoid congestion, Vegas linearly increases/decreases its congestion window to ensure
 * the diff value fall between the 2 predefined thresholds, alpha and beta.
 * diff and another threshold, gamma, are used to determine when Vegas should change from
 * its slow-start mode to linear increase/decrease mode.
 *
 * Following the implementation of Vegas in Linux, we use 2, 4, and 1 as the default values
 * of alpha, beta, and gamma, respectively.
 *
 * More information: http://dx.doi.org/10.1109/49.464716
 */

class TcpCopa : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create an unbound tcp socket.
   */
  TcpCopa (void);

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpCopa (const TcpCopa& sock);
  virtual ~TcpCopa (void);

  virtual std::string GetName () const;

  /**
   * \brief Compute RTTs needed to execute Vegas algorithm
   *
   * The function filters RTT samples from the last RTT to find
   * the current smallest propagation delay + queueing delay (minRtt).
   * We take the minimum to avoid the effects of delayed ACKs.
   *
   * The function also min-filters all RTT measurements seen to find the
   * propagation delay (baseRtt).
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   * \param rtt last RTT
   *
   */
  virtual void Send(Ptr<TcpSocketBase> tsb, Ptr<TcpSocketState> tcb,
                      SequenceNumber32 seq, bool isRetrans);

  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);
  virtual void PktsAckedACopa (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                            const Time& rtt);

  /**
   * \brief Enable/disable Vegas algorithm depending on the congestion state
   *
   * We only start a Vegas cycle when we are in normal congestion state (CA_OPEN state).
   *
   * \param tcb internal congestion state
   * \param newState new congestion state to which the TCP is going to switch
   */
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
  virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);

  /**
   * \brief Adjust cwnd following Vegas linear increase/decrease algorithm
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   */
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  virtual void ExitRecovery (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) ;

  virtual void copaModifyWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  /**
   * \brief Get slow start threshold following Vegas principle
   *
   * \param tcb internal congestion state
   * \param bytesInFlight bytes in flight
   *
   * \return the slow start threshold value
   */
  
  virtual Ptr<TcpCongestionOps> Fork ();

protected:
private:
  /**
   * \brief Enable Vegas algorithm to start taking Vegas samples
   *
   * Vegas algorithm is enabled in the following situations:
   * 1. at the establishment of a connection
   * 2. after an RTO
   * 3. after fast recovery
   * 4. when an idle connection is restarted
   *
   * \param tcb internal congestion state
   */
  void EnableCopa (Ptr<TcpSocketState> tcb);

  /**
   * \brief Stop taking Vegas samples
   */
  void DisableCopa ();

  void opModeForCopaplus(Ptr<TcpSocketState> tcb, const Time& rtt);
  bool modeSwitch(Ptr<TcpSocketState> tcb, const Time& rtt);
  double calculateAlpha(Ptr<const TcpSocketState> tcb);
  void IncreaseWindow_perAck (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

private:
  double m_delta;                   //!< the target numble of packet queue in the buffer
  uint32_t m_v;                  //!< velocity parameter for increase
  Time m_srtt;	                      // smooth rtt
  uint32_t m_cntRtt;                 //!< Number of RTT measurements during last RTT
  bool m_doingCopaNow;              //!< If true, do Copa for this RTT
  SequenceNumber32 m_begSndNxt;      //!< Right edge during last RTT

  Time m_RTTmin;
  Time recordTimeMin;
  Time m_RTTstanding,m_Tmin,m_Tmax;
  Time m_oldRTTstanding;
  Time recordTimeStanding;

  double oldCwndpkts;
  double newCwndpkts;
  double oldAckTime;
  double oldAckSeq;

  double pra;
  
  std::map<SequenceNumber32,uint32_t> cwndHistory;
  std::map<SequenceNumber32,Time> ackTimeHistory;
  std::map<SequenceNumber32,SequenceNumber32> ackSeqHistory;

  Time oldRtt1;
  Time oldRtt2;

  bool oldDirect;
  bool newDirect;
  uint32_t times;
  bool isFirstTime;

  bool enableComp,isStrawman,iscopaModify,cpModeInitial,iniProc;
  uint32_t cpVersion,m_Wmax,rttInc,cwndInc,cwndTot,cwndBeforeCo,old_cwnd,cpCnt,thrTemp,thrMax;
  Time tZero,tLoss,tDeltaUpdate,m_itv, lastCheck, old_rtt;
  double rttAvg,m_alpha,m_gama,m_defaultDelta;
  std::vector<double> rttArray;
  std::vector<double> cwndArray;




  Time Qtarget;

  bool isSlowStart;

  Time m_RTTmax,globalRttMax;
  bool tempEmpty,compEmpty;
  bool defaultMode;
  uint32_t RTTCountForEmpty;
  uint32_t RTTCountForMax;
  double RTTHistory[4]={0,0,0,0};
  bool emptyHistory[5]={false,false,false,false,false};
  Time tempMax;
  bool nearEmpty;
  double assitPra;
  double Epra;
  double oldEpra;
  double v0;
  uint32_t RTTCountFormin;
  double NEpra;
  uint32_t RTTCount;
  bool RTTdirect;
  bool isBiTheta;
  double ssTheta;
  double sstarget;
  double Smax;
  Time recordTimeEmpty;
  bool directChange;
  uint32_t RTTforCountDirectChange;
  uint32_t ForEmpty;
  double oldCwndMid;
  Time m_RTTq;
  uint32_t ackCount;
  double intervalPoint;
  double srttPoint;
  double simiLast;
  std::deque<std::pair<int64_t, ns3::Time>> queMin,queStd,queTmin,queTmax;
};

} // namespace ns3

#endif // TCPCOPA_H
