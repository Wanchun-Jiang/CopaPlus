#undef NDEBUG // We want the assert statements to work

#include "ADC.hh"
#include <stdio.h>
#include <math.h>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
//#define LDEBUG

using namespace std;

int AdaptiveCC::flow_id_counter = 0;

double AdaptiveCC::current_timestamp( void ){
  return cur_tick;
}

void AdaptiveCC::init() {
  if (num_pkts_acked != 0) {
    cout << "% Packets Lost: " << (100.0 * num_pkts_lost) /
      (num_pkts_acked + num_pkts_lost) << " at " << current_timestamp() << " " << num_pkts_acked << " " << num_pkts_sent << " min_rtt= " << min_rtt << " " << num_pkts_acked << " " << num_pkts_lost << endl;
    if (slow_start)
      cout << "Finished while in slow-start at window " << _the_window << endl;
  }

  _intersend_time = 0;
  _the_window = num_probe_pkts;
  if (external_min_rtt != 0)
    _intersend_time = external_min_rtt / _the_window;
  if (keep_ext_min_rtt)
    min_rtt = external_min_rtt;
  else
    min_rtt = numeric_limits<double>::max();
  _timeout = 1000;
  
  if (utility_mode != CONSTANT_DELTA)
    delta = default_delta;
  
  unacknowledged_packets.clear();

  rtt_unacked.reset();
  rtt_window.clear();
  prev_intersend_time = 0;
  cur_intersend_time = 0;
  interarrival.reset();
  is_uniform.reset();

  
  last_update_time = 0;
  update_dir = 1;
  prev_update_dir = 1;
  pkts_per_rtt = 0;
  update_amt = theta = 1;
  
  intersend_time_vel = 0;
  prev_intersend_time_vel = 0;
  prev_rtt = 0;
  prev_rtt_update_time = 0;
  prev_avg_sending_rate = 0;

  num_pkts_acked = num_pkts_lost = num_pkts_sent = 0;
  operation_mode = DEFAULT_MODE;
  flow_length = 0;
  prev_delta_update_time = 0;
  prev_delta_update_time_loss = 0;
  max_queuing_delay_estimate = 0;

  loss_rate.reset();
  reduce_on_loss.reset();
  loss_in_last_rtt = false;
  interarrival_ewma.reset();
  prev_ack_time = 0.0;
  exp_increment = 1.0;
  prev_delta.reset();
  slow_start = true;
  slow_start_threshold = 1e10;
  recordCycle = -1;
  recordEmpty = -1;
  oldRttStanding = -1;
  simiLast = 1;

  cycle=1.0;

  Qd_array.clear();
  cwnd_array.clear();
  time_array.clear();
  packet_acked_time_array.clear();
  cout << "init FD_threshold = " <<FD_threshold<<endl;
  cout << "init TD_threshold = " <<TD_threshold<<endl;
  if (FD_threshold == 0)
  	FD_threshold = 0.02;
  if (TD_threshold == 0)
        TD_threshold = 0.7;
  probe_cycle_start = 0;
  rttInc=0;
  cwndInc=0;
  preCwnd=0;
  preRTT=0;
  global_rtt_min = 0x3f3f3f3f;
  operaton_point = 1000000.0;
  rtt_max = 0;
  P_ave = 0;
  P_cnt = 0;

}



bool AdaptiveCC::FFT()
{
    int n = Qd_array.size();
    //double fs=1.0*size/cycle;
    double temp,qst=-1;
    //rtt_num++;
    fftw_complex *outa,*ina;
    outa = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
    ina = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * n);
    double *cwnd_amp = new double[n];
    double similarity=0;
    //cwnd fft
    for(int i=0;i<n;i++)
    {
        ina[i][0]=cwnd_array[i];
        ina[i][1]=0;
    }
    fftw_plan ppp=fftw_plan_dft_1d(n,ina,outa,FFTW_FORWARD,FFTW_ESTIMATE);
    fftw_execute(ppp);
    fftw_destroy_plan(ppp);
    double PP_ave=sqrt(outa[0][0]*outa[0][0]+outa[0][1]*outa[0][1]);
    cwnd_amp[0]=1;
    for(int i=1;i<n/2;i++)
    {
        cwnd_amp[i]=sqrt(outa[i][0]*outa[i][0]+outa[i][1]*outa[i][1])*2/PP_ave;
        //cout << rtt_num << "cwndFFT: " << i*fs/n << " " << temp*2/n <<endl;//^!^
    }
    //Qd fft
    for(int i=0;i<n;i++)
    {
        ina[i][0]=Qd_array[i];
        ina[i][1]=0;
    }
    fftw_plan pp=fftw_plan_dft_1d(n,ina,outa,FFTW_FORWARD,FFTW_ESTIMATE);
    fftw_execute(pp);
    fftw_destroy_plan(pp);
    PP_ave=sqrt(outa[0][0]*outa[0][0]+outa[0][1]*outa[0][1]);
    for(int i=1;i<n/2;i++)
    {
        temp=sqrt(outa[i][0]*outa[i][0]+outa[i][1]*outa[i][1]);
        //cout << rtt_num << "qdFFT: " << i*fs/n << " " << temp*2/n <<endl;//^!^
        if(qst<temp)
        {
            qst=temp;
            //fm=i;
        }
        temp=temp*2/PP_ave;
        similarity+=fabs(temp-cwnd_amp[i]);
    }
    fftw_free(outa);
    fftw_free(ina);
    delete cwnd_amp;
    similarity/=n;

    double simiD = 0.0;
    simiD=abs(double(1.0*(rttInc-cwndInc)))/n;

    #ifdef LDEBUG
    cerr <<"FFT_Result: simiD "  << simiD << " similarity " << similarity << " smoothSimi " << simiLast << " rttInc " << rttInc << " cwndInc " << cwndInc << " timing " << probe_cycle_start << " cycle " << cycle <<endl; 
    #endif

    if(similarity > FD_threshold /*(similarity > 5*simiLast)*/ || simiD > TD_threshold){
        //simiLast = similarity;
        return true;
    } else {
      // if (iniProc) iniProc = false;
      // else if(simiLast == 1.0) simiLast = similarity;
      // else simiLast = simiLast/2 + similarity/2;
	return false;
    }
}


void AdaptiveCC::update_theta()
{
    double cur_time = current_timestamp();
    double rttStanding = rtt_window.get_unjittered_rtt();
    if(rtt_window.is_copa()) tempEmpty = true;
    //if(rttStanding-min_rtt <= 0.01*min_rtt) tempEmpty = true;
    // dectect cycle per 0.5srtt
    //if(recordCycle + rtt_window.get_srtt() >= cur_time)
    if(oldRttStanding != rttStanding)//lhy
    {
        // judge the direction change
        if(rttStanding > oldRttStanding)
        {
            if(!rttDirect)
            {
                if(tempEmpty)
                {
                    cntRttEmpty++;
                }
                cntRttChange++;
            }
            rttDirect = true;
        }
        else
        {
            if(rttDirect)
            {
                if(tempEmpty)
                {
                    cntRttEmpty++;
                }
                cntRttChange++;
            }
            rttDirect = false;
        }
        oldRttStanding = rttStanding;
        recordCycle = cur_time;
    }
    
    
    bool nearEmpty = tempEmpty;
    if (cntRttChange >= 2 && cntRttChange%2 == 0 && cntRttEmpty >= 2 && cntRttEmpty%2 == 0) tempEmpty = false;
    
    // update theta
    if (update_amt > 1) cntRttChange = 0;//lhy
    if (cntRttChange >= 2 && update_amt == 1)
    {
        if (nearEmpty && cntRttEmpty >= thrEmpty)
        {
            recordEmpty = cur_time;
            eTheta = theta;
            theta *= 0.75;
            thrEmpty *= 2; 
            cntRttChange = cntRttEmpty = 0;
        }
        else if (!nearEmpty && cntRttChange % 2 == 0)
        {
            thrEmpty = 2;
            if (recordEmpty == -1) recordEmpty = cur_time;
            double K=std::pow(2.5*eTheta,1.0/3.0);
            double T_K=std::pow((0.001*(cur_time-recordEmpty)-K),3);
            theta = 0.1 * T_K + eTheta;
            theta = std::min(theta, _the_window);
            cntRttEmpty = 0;
        }
    }
}


void AdaptiveCC::update_delta(bool pkt_lost __attribute((unused)), double cur_rtt) {
  double cur_time = current_timestamp();
  if (utility_mode == AUTO_MODE) {
    if (pkt_lost) {
      is_uniform.update(rtt_window.get_unjittered_rtt());
      //cout << "Packet lost: " << cur_time << endl;
    }
   // if (!rtt_window.is_copa() && cntRttChange == 0) {
   if (!tempEmpty){
      operation_mode = LOSS_SENSITIVE_MODE;
    }
    else {
      operation_mode = DEFAULT_MODE;
    }
  }

  if (operation_mode == DEFAULT_MODE && utility_mode != TCP_COOP) {
    if (prev_delta_update_time == 0. || prev_delta_update_time_loss + cur_rtt < cur_time) {
      if (delta < default_delta)
        delta = default_delta; //1. / (1. / delta - 1.);
      delta = min(delta, default_delta);
      prev_delta_update_time = cur_time;
    }
  }
  else if (utility_mode == TCP_COOP || operation_mode == LOSS_SENSITIVE_MODE) {
    if (prev_delta_update_time == 0)
      delta = default_delta;
    //if (pkt_lost && prev_delta_update_time_loss + cur_rtt < cur_time) {
    if (prev_delta_update_time_loss + cur_rtt < cur_time && (cntRttChange == 0 || pkt_lost)){ // restraint of competetive mode
      delta *= 2;
      // double decrease = 0.5 * _the_window * rtt_window.get_min_rtt() / (rtt_window.get_unjittered_rtt());
      // if (decrease >= 1. / (2. * delta))
      // 	delta = default_delta;
      // else
      // 	delta = 1. / (1. / (2. * delta) - decrease);
      prev_delta_update_time_loss = cur_time;
    }
    else {
      if (prev_delta_update_time + cur_rtt < cur_time) {
        delta = 1. / (1. / delta + 1.);
        //cout << "DU " << cur_time << " " << flow_id << " " << delta << endl;
        prev_delta_update_time = cur_time;
      }
    }
    delta = min(delta, default_delta);
  }
}

double AdaptiveCC::randomize_intersend(double intersend) {
  return intersend;
}


void AdaptiveCC::update_intersend_time() {
  double cur_time __attribute((unused)) = current_timestamp();
  // if (external_min_rtt == 0) {
  //   cout << "External min. RTT estimate required." << endl;
  //   exit(1);
  // }
  
  // First two RTTs are for probing
  if (num_pkts_acked < 2 * num_probe_pkts - 1)
    return;

  // Calculate useful quantities
  double rtt = rtt_window.get_unjittered_rtt();
  double queuing_delay = rtt - min_rtt;

  double target_window;
  if (queuing_delay == 0)
    target_window = numeric_limits<double>::max();
  else
    target_window = rtt / (queuing_delay * delta);

  // Handle start-up behavior
  if (slow_start) {
    if (do_slow_start || target_window == numeric_limits<double>::max()) {
      _the_window += 1;
      if (_the_window >= target_window)
        slow_start = false;
    }
    else {
      assert(false);
      // _the_window = rtt / ((min_rtt + rtt_window.get_max()) * 0.5 - min_rtt);
      // cout << "Fast Start: " << rtt_window.get_min() << " " << rtt_window.get_max() << " " << _the_window << endl;
      // slow_start = false;
    }
  }
  // Update the window
  else {
    if (last_update_time + rtt_window.get_latest_rtt() < cur_time) {
      if (prev_update_dir * update_dir > 0) {
        if (update_amt < 0.006)
          update_amt += 0.005;
        else
          update_amt = (int)update_amt * 2;
      }
      else {
        update_amt = 1.;
        prev_update_dir *= -1;
      }
      last_update_time = cur_time;
      pkts_per_rtt = update_dir = 0;
    }

    if(compete_mode){
	    update_amt = 1;
	    theta = 1;
    } else {
	    alpha = 1;
    }

    if (update_amt * theta > _the_window * delta ) {
      update_amt /= 2;
    }

    update_amt = max(update_amt, 1.);
    ++ pkts_per_rtt;

    if (_the_window < target_window || compete_mode ){
       ++ update_dir;
      _the_window += update_amt * theta * alpha / (delta * _the_window);
    }
    else {
      -- update_dir;
      _the_window -= update_amt * theta * alpha / (delta * _the_window);
    }
  }

  // Set intersend time and perform boundary checks.
  _the_window = max(2.0, _the_window);
  cur_intersend_time = 0.5 * rtt / _the_window;
  
  #ifdef LDEBUG
  cerr << "time= " << cur_time << " window= " << _the_window << " target= " << target_window << " rtt= " << rtt << " min_rtt= " << min_rtt << " delta= " << delta << " update_amt= " << update_amt << " compete_mode= " << compete_mode /* << " update_dir " << update_dir << " cycle " << bic_k*/ << " theta= " << theta << endl;
  #endif

  //std::cerr << "time= " << cur_time << " window= " << _the_window << " target= " << target_window << " rtt= " << rtt << " min_rtt= " << min_rtt << " delta= " << delta << " update_amt= " << update_amt << " Qd= " << queuing_delay << " theta= " << theta << " nearEmpty?" << tempEmpty << endl;
  _intersend_time = randomize_intersend(cur_intersend_time);
}

void AdaptiveCC::onACK(int ack, 
			double receiver_timestamp __attribute((unused)),
			double sent_time, int delta_class __attribute((unused))) {
  int seq_num = ack - 1;
  double cur_time = current_timestamp();
  assert(cur_time > sent_time);


  double rttInst = cur_time - sent_time;

  
  rtt_window.new_rtt_sample(rttInst, cur_time);
  //min_rtt = rtt_window.get_min_rtt();
  //change for TCP competitive mode by np 1.13
  if(delta == default_delta){
      min_rtt = rtt_window.get_min_rtt();
  } else {
      min_rtt = min(min_rtt,rttInst);
  }
  if (srtt != 0 ){
	srtt = (1.0 /16.0) * rttInst + (15.0/16.0)*srtt;
  } else {
	srtt = rttInst;
  }
  min_rtt = min(global_rtt_min,min_rtt);
  global_rtt_min = min(global_rtt_min,min_rtt);
  min_rtt = global_rtt_min;

  if(cycle == 1.0) cycle= std::max(1.0, 20.0 * min_rtt/1000.0);

  assert(rtt_window.get_unjittered_rtt() >= min_rtt);

  
  /*while(!compete_mode && !operating_Qd_array.empty() && cur_time - operating_Qd_array.front().second > 5 * srtt){
        sum_operating_Qd = sum_operating_Qd - operating_Qd_array.front().first;
        sum_operating_rtt = sum_operating_rtt -  operating_rtt_array.front().first;
        sum_operating_cwnd = sum_operating_cwnd -  operating_cwnd_array.front().first;
        count_operating_Qd --;
        operating_Qd_array.pop_front();
        operating_rtt_array.pop_front();
        operating_cwnd_array.pop_front();
  }
  if (!compete_mode && cur_time > 2000){
        operating_Qd_array.push_back(std::make_pair(rttInst - min_rtt,cur_time));
        operating_rtt_array.push_back(std::make_pair(rttInst,cur_time));
        operating_cwnd_array.push_back(std::make_pair(_the_window,cur_time));
        sum_operating_Qd += rttInst - min_rtt;
        sum_operating_rtt +=rttInst;
        sum_operating_cwnd +=_the_window;
        count_operating_Qd ++;
        if (operaton_point > sum_operating_Qd / count_operating_Qd){
                operaton_point = sum_operating_Qd / count_operating_Qd;
                rtt_min_operaton_point_ = min_rtt;
                operating_rtt_avg = sum_operating_rtt / count_operating_Qd;
                operating_cwnd_avg = sum_operating_cwnd / count_operating_Qd;
        }
  }else{
        operating_Qd_array.clear();
	operating_rtt_array.clear();
	operating_cwnd_array.clear();
        sum_operating_Qd = 0.0;
	sum_operating_rtt = 0.0;
	sum_operating_cwnd = 0.0;
        count_operating_Qd = 0;
  }*/  

  if(compete_mode){
  	RTTavg = RTTavg + rttInst;
	RTTcount++;
  }

  if(recordCycle == -1)
  {
      recordCycle = cur_time;
      oldRttStanding = rtt_window.get_unjittered_rtt();
      cntRttChange = cntRttEmpty = 0;
      thrEmpty = 2;
      theta = eTheta = 1;
  }

  if (prev_ack_time != 0) {
    interarrival_ewma.update(cur_time - prev_ack_time, cur_time / min_rtt);
    interarrival.push(cur_time - prev_ack_time);
  }

  Time rtt_measured= rtt_window.get_srtt();
  if(rtt_measured < 500.0) //error vulue
  	rtt_max = max(rtt_max,rtt_measured);
  prev_ack_time = cur_time;
  
  if(!compete_mode) update_theta();
  else theta=1;

  if (probe_cycle_start == 0){
  	probe_cycle_start = cur_time;
	//cout<< "probe_cycle_start"<<endl;
  }

  // for tcp competitive mode by np 2022.1.13
  //update_delta(false, rttInst);

  
  bool pkt_lost = false;
  bool reduce = false;
  int it_cnt=0,it_tot=unacknowledged_packets.count(seq_num);
  if (unacknowledged_packets.count(seq_num) != 0) {
    int tmp_seq_num = -1;
    for (std::map<SeqNum, PacketData>::iterator x=unacknowledged_packets.begin();x!=unacknowledged_packets.end();) {
          if(it_cnt>it_tot)break;
          assert(tmp_seq_num <= x->first);
      tmp_seq_num = x->first;
      if (x->first > seq_num)
        break;
      prev_intersend_time = x->second.intersend_time;
      prev_intersend_time_vel = x->second.intersend_time_vel;
      prev_rtt = x->second.rtt;
      prev_rtt_update_time = x->second.sent_time;
      prev_avg_sending_rate = x->second.prev_avg_sending_rate;
      if (x->first < seq_num) {
        ++ num_pkts_lost;
        pkt_lost = true;
        reduce |= reduce_on_loss.update(true, cur_time, rtt_window.get_latest_rtt());
      }
      unacknowledged_packets.erase(x++);
    }
  }

  // reduce |= reduce_on_loss.update(false, cur_time, rtt_window.get_latest_rtt());
  // if (reduce && !compete_mode) {
  //   _the_window *= 0.7;
  //   _the_window = max(2.0, _the_window);
  //   cur_intersend_time = 0.5 * rtt_window.get_unjittered_rtt() / _the_window;
  //   _intersend_time = randomize_intersend(cur_intersend_time);
  // }

  /*if(compete_mode) {
  	cout << "current_time "<< cur_time << " smooth_pkt_loss_interval "<<  smooth_pkt_loss_interval << " now_smooth_pkt_loss_interval " << smooth_pkt_loss_interval * 0.5 + (current_timestamp() - pkt_loss_time) * 0.5 << " now_cwnd " << _the_window << " last_cwnd " << last_max_cwnd <<endl;
  
  }*/

  if(compete_mode && pkt_loss_time > 0.0 && smooth_pkt_loss_interval > 0.0 && smooth_pkt_loss_interval * 0.5 + (current_timestamp() - pkt_loss_time) * 0.5 > 1.5 * smooth_pkt_loss_interval && _the_window > last_max_cwnd){ // change itv factor from 2 -> sqrt3(2) by lhy 2022.6.28
	pkt_loss_time = 0.0;
	last_cwnd_avg = cwnd_avg;
      	last_rtt_avg = rtt_avg;
      	cwnd_avg = 0.0;
      	rtt_avg = 0.0;
      	cwnd_array.clear();
      	Qd_array.clear();
      	P_ave=0;
      	P_cnt=0;
      	rttInc=0;
      	cwndInc=0;
      	time_array.clear();
	probe_cycle_start = cur_time;
	alpha = 1;
	smooth_pkt_loss_interval = 0.0;
	compete_mode = false;

  //_the_window = (operating_cwnd_avg /operating_rtt_avg) * min_rtt; // test by lhy 0629
  iniProc = true;

	cycle= 20.0 * min_rtt/1000.0;
	recordCycle = cur_time;
        oldRttStanding = rtt_window.get_unjittered_rtt();
        cntRttChange = cntRttEmpty = 0;
        thrEmpty = 2;
        theta = eTheta = 1;
  }

  if (/*!compete_mode && */probe_cycle_start < cur_time)
  {
      P_cnt++;

        
      if(rtt_measured >= preRTT)rttInc++;
      if(_the_window > preCwnd)cwndInc++;
      preRTT=rtt_measured;
      preCwnd=_the_window;
      cwnd_array.push_back(_the_window);
      Qd_array.push_back(rtt_measured);
      time_array.push_back(cur_time);
      P_ave+=(rtt_measured);
      
      cwnd_avg += _the_window;
      rtt_avg += rtt_measured;
      rtt_max = max(rtt_max,rtt_measured);
      //cout<< "current_time "<< cur_time << " P_ave "<<P_ave << " P_cnt " << P_cnt << " rttInc "<< rttInc << " cwndInc " << cwndInc <<endl;
  }

  //For TCP competitive mode by 2022.1.12

  if (probe_cycle_start+cycle*1000 < cur_time /*&& !compete_mode*/)
  {
      
      operating_cwnd_avg = std::max(1.0*P_cnt / (cur_time - probe_cycle_start),operating_cwnd_avg);
      #ifdef LDEBUG
        std::cerr<< "Debug: update reciving rate at "<< cur_time << " P_cnt " << P_cnt << " itv "<< cur_time - probe_cycle_start << " maxRecvRate(inWindow) "<< operating_cwnd_avg <<std::endl;
      #endif

      probe_cycle_start=cur_time;

      cwnd_avg /= P_cnt;
      rtt_avg /= P_cnt;
      if (!compete_mode) 
      {
        compete_mode = FFT();
        //compete_mode = false; 

        // if(/*!compete_mode && */(operating_rtt_avg == 0 || (operating_cwnd_avg / operating_rtt_avg ) < (cwnd_avg / rtt_avg))){ // clear thrMax by lhy 0624
        //   operating_cwnd_avg = cwnd_avg;
        //   operating_rtt_avg = rtt_avg;
        // }

      

        if (compete_mode)
          pkt_loss_time = cur_time;
        P_ave = P_ave * (1.0 / P_cnt);
        if(!Qd_array.empty() && compete_mode && utility_mode == AUTO_MODE)
          {
  	       //_the_window = std::max((operating_cwnd_avg /operating_rtt_avg) * rtt_max, _the_window); // remove by lhy 0620
            _the_window = std::max((operating_cwnd_avg) * rtt_max, _the_window); // remove by lhy 0629
            //_the_window = std::max(last_cwnd_avg,cwnd_avg);
  	        pkt_loss_time = current_timestamp();
            last_max_cwnd = _the_window;
            //_the_window *= 0.85;

            operating_cwnd_avg = operating_rtt_avg = 0; // clear thrMax by lhy 0624
            rtt_max = 0;
  	  double beta = 717.0 / 1024.0;
            //_the_window = max(2.0, 17.0/20.0 * last_max_cwnd);
            double C = 410.0 / 1024.0;
            bic_k = pow((last_max_cwnd *(1.0-beta))/C, 1.0/3.0) * 1000.0;
  	        //smooth_pkt_loss_interval = bic_k / 2.0;

          }else{
              delta = default_delta;
    	       cycle= 20.0 * min_rtt/1000.0;
          }

        #ifdef LDEBUG
        std::cerr<< "FFTcurrent_time "<< cur_time << " P_ave "<<P_ave << " P_cnt " << P_cnt << " rttInc "<< rttInc << " cwndInc " << cwndInc << " cycle " << cycle << " compete_mode "<< compete_mode << " delta " << delta << " rtt_max " << rtt_max << " rtt_min_operaton_point_ " << rtt_min_operaton_point_ << " operaton_point " << operaton_point << " cwnd_avg " << cwnd_avg << " last_cwnd_avg " << last_cwnd_avg << " rtt_avg " << rtt_avg << " last_rtt_avg " << last_rtt_avg << " initialization: cwnd,rtt " << operating_cwnd_avg << " "<< operating_rtt_avg << endl;
        //std::cerr<<" P_ave "<<P_ave<<" P_ave_pre "<<P_ave_pre << " Qamp " << Qamp << " gain "<< gain << " RTT_max " << max_rtt << " if_safe " << nearEmpty << probe_ceil << " C/N " << lamda*(cur_time - sent_time)/Qamp << " Bd " << Bd <<endl;
        #endif
      }

      if(P_cnt) last_cwnd_avg = cwnd_avg;
      last_rtt_avg = rtt_avg;
      cwnd_avg = 0.0;
      rtt_avg = 0.0;
      cwnd_array.clear();
      Qd_array.clear();
      P_cnt=0;
      P_ave=0;
      rttInc=0;
      cwndInc=0;
      time_array.clear();
  }

  update_intersend_time();

   if(compete_mode && pkt_lost && cur_time - pkt_loss_time > rtt_window.get_unjittered_rtt()){
          
	if(smooth_pkt_loss_interval == 0.0){
		smooth_pkt_loss_interval = current_timestamp() - pkt_loss_time; 
	} else {
		smooth_pkt_loss_interval = smooth_pkt_loss_interval * 0.5 + (current_timestamp() - pkt_loss_time) * 0.5;
	}

	RTTavg = RTTavg / RTTcount;


	pkt_loss_time = current_timestamp();
        last_max_cwnd = _the_window;
        double beta = 717.0 / 1024.0;
        _the_window = max(2.0, 17.0/20.0 * _the_window);
        double C = 410.0 / 1024.0;
	bic_k = pow((last_max_cwnd *(1.0-beta))/C, 1.0/3.0) * 1000.0;

	alpha = last_max_cwnd * (3.0/20.0)  * delta * RTTavg / bic_k ;
  
  #ifdef LDEBUG
	cerr<<"pkt_lost time " <<pkt_loss_time << " bic_k " << bic_k << " alpha " << alpha << " delta " << delta << " last_max_cwnd " << last_max_cwnd << " cwnd " << _the_window  <<endl;
	#endif

  cur_intersend_time = 0.5 * rtt_window.get_unjittered_rtt() / _the_window;
        _intersend_time = randomize_intersend(cur_intersend_time);

	RTTavg = 0.0;
	RTTcount = 0;
   }

  ++ num_pkts_acked;
}

void AdaptiveCC::onPktSent(int seq_num) {
  double cur_time = current_timestamp();
  // double tmp_prev_avg_sending_rate = 0.0;
  // if (prev_intersend_time != 0.0)
  //   tmp_prev_avg_sending_rate = 1.0 / prev_intersend_time;
  unacknowledged_packets[seq_num] = {cur_time,
                                     cur_intersend_time,
                                     intersend_time_vel,
                                     rtt_window.get_unjittered_rtt(),
                                     unacknowledged_packets.size() / (cur_time - prev_rtt_update_time)
  };

  rtt_unacked.force_set(rtt_window.get_unjittered_rtt(), cur_time / min_rtt);
  for (auto & x : unacknowledged_packets) {
    if (cur_time - x.second.sent_time > rtt_unacked) {
      rtt_unacked.update(cur_time - x.second.sent_time, cur_time / min_rtt);
      prev_intersend_time = x.second.intersend_time;
      prev_intersend_time_vel = x.second.intersend_time_vel;
      continue;
    }
    break;
  }
  //update_intersend_time();
  ++ num_pkts_sent;

  _intersend_time = randomize_intersend(cur_intersend_time);
}

void AdaptiveCC::close() {
}

void AdaptiveCC::onDupACK() {
  ///num_pkts_lost ++;
  // loss_rate = 1.0 * alpha_loss + loss_rate * (1.0 - alpha_loss);
  //slow_start = false;
  //update_delta(true);
  //
  //calculte alpha

}

void AdaptiveCC::onTimeout() {
  //num_pkts_lost ++;
  // loss_rate = 1.0 * alpha_loss + loss_rate * (1.0 - alpha_loss);
  //cout << "Timeout\n";
  //slow_start = false;
  //update_delta(true);
}

void AdaptiveCC::interpret_config_str(string config) {
  // Format - [do_ss:][keep_ext_min_rtt:]delta_update_type:param1:param2...
  // Delta update types:
  //	 -- constant_delta - params:- delta value
  //	 -- pfabric_fct - params:- none
  //	 -- bounded_delay - params:- delay bound (s)
  //	 -- bounded_delay_end - params:- delay bound (s), done in an end-to-end manner

  if (config.substr(0, 6) == "do_ss:") {
    do_slow_start = true;
    config = config.substr(6, string::npos);
    cout << "Will do slow start" << endl;
  }
  if (config.substr(0, 17) == "keep_ext_min_rtt:") {
    keep_ext_min_rtt = true;
    config = config.substr(17, string::npos);
    cout << "Will keep external Min. RTT" << endl;
  }
  
  if (config.substr(0, 3) == "fd:"){
     int pos = config.find(':',3);
     FD_threshold= stof(config.substr(3, pos-3).c_str());  
     config = config.substr(pos+1, string::npos);
     cout << " FD_threshold = "<< FD_threshold <<endl;
  }

  if (config.substr(0, 3) == "td:"){
     int pos = config.find(':',3);
     TD_threshold= stof(config.substr(3, pos-3).c_str());
     config = config.substr(pos+1, string::npos);
     cout << " TD_threshold = "<< TD_threshold <<endl;
  }


  // if (config.substr(0, 14) == "default_delta:") {
  //   keep_ext_min_rtt = true;
  //   default_delta = atof(config.substr(14, string::npos).c_str());
  //   default_delta = config.substr(14, string::npos);
  //   cout << "Will use a default delta of " << default_delta << endl;
  // }

  delta = 1; // If delta is not set in time, we don't want it to be 0
  if (config.substr(0, 15) == "constant_delta:") {
    utility_mode = CONSTANT_DELTA;
    delta = atof(config.substr(15, string::npos).c_str());
    cout << "Constant delta mode with delta = " << delta << endl;
  }
  else if (config.substr(0, 11) == "pfabric_fct") {
    utility_mode = PFABRIC_FCT;
    cout << "Minimizing FCT PFabric style" << endl;
  }
  else if (config.substr(0, 14) == "bounded_delay:") {
    utility_mode = BOUNDED_DELAY;
    delay_bound = stof(config.substr(14, string::npos).c_str());
    cout << "Bounding delay to " << delay_bound << " s" << endl;
  }
  else if (config.substr(0, 18) == "bounded_delay_end:") {
    utility_mode = BOUNDED_DELAY_END;
    delay_bound = stof(config.substr(18, string::npos).c_str());
    cout << "Bounding delay to " << delay_bound << " s in an end-to-end manner" << endl;
  }
  else if (config.substr(0, 19) == "bounded_qdelay_end:") {
    utility_mode = BOUNDED_QDELAY_END;
    delay_bound = stof(config.substr(19, string::npos).c_str());
    cout << "Bounding queuing delay to " << delay_bound << " s in an end-to-end manner" << endl;
  }
  else if (config.substr(0, 18) == "bounded_fdelay_end:") {
    utility_mode = BOUNDED_FDELAY_END;
    delay_bound = stof(config.substr(18, string::npos).c_str());
    cout << "Bounding fractional queuing delay to " << delay_bound << " s in an end-to-end manner" << endl;
  }
  else if (config.substr(0, 14) == "max_throughput") {
    utility_mode = MAX_THROUGHPUT;
    cout << "Maximizing throughput" << endl;
  }
  else if (config.substr(0, 16) == "different_deltas") {
    utility_mode = CONSTANT_DELTA;
    delta = flow_id * 0.5;
    cout << "Setting constant delta to " << delta << endl;
  }
  else if (config.substr(0, 8) == "tcp_coop") {
    utility_mode = TCP_COOP;
    cout << "Cooperating with TCP" << delta << endl;
  }
  else if (config.substr(0, 14) == "const_behavior") {
    utility_mode = CONST_BEHAVIOR;
    behavior = stof(config.substr(15, string::npos).c_str());
    cout << "Exhibiting constant behavior " << behavior << endl;
  }
  else if (config.substr(0, 5) == "auto:") {
    utility_mode = AUTO_MODE;
    default_delta = atof(config.substr(5, string::npos).c_str());
    delta = default_delta;
    cerr << "Using Automatic Mode with default delta = " << default_delta << endl;
  }
  else {
    utility_mode = CONSTANT_DELTA;
    delta = 1.0;
    cout << "Incorrect format of configuration string '" << config
	 << "'. Using constant delta mode with delta = 1 by default" << endl;
  }
}
