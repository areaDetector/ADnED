
/**
 * @brief nED V4 channel code to subscribe to V4 PV updates
 *
 * @author Matt Pearson
 * @date Sept 2014
 */

#ifndef NEDCHANNEL_H
#define NEDCHANNEL_H

#include <epicsThread.h>
#include <epicsTime.h>
#include <pv/epicsException.h>
#include <pv/createRequest.h>
#include <pv/event.h>
#include <pv/pvData.h>
#include <pv/clientFactory.h>
#include <pv/pvAccess.h>
#include <pv/monitor.h>

namespace nEDChannel {

  using namespace std::tr1;
  using namespace epics::pvData;
  using namespace epics::pvAccess;

  //Channel connect/disconnect class

  class nEDChannelRequester : public virtual ChannelRequester {

  public:
    
    nEDChannelRequester(std::string &requester_name);
    virtual ~nEDChannelRequester();
    
    void channelCreated(const Status& status, Channel::shared_pointer const & channel);
    void channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState);
    boolean waitUntilConnected(double timeOut);
    std::string getRequesterName();
    void message(std::string const & message, MessageType messageType);
    
  private:
    
    std::string m_requesterName;
    Event m_connectEvent;
    
  };
 
  //Channel monitor class

  class nEDMonitorRequester : public virtual MonitorRequester
  {
    
  public:
    
    nEDMonitorRequester(std::string &requester_name);
    virtual ~nEDMonitorRequester();
    
    void monitorConnect(Status const & status, MonitorPtr const & monitor, StructureConstPtr const & structure);
    void monitorEvent(MonitorPtr const & monitor);
    void unlisten(MonitorPtr const & monitor);
    boolean waitUntilDone();

    std::string getRequesterName();
    void message(std::string const & message, MessageType messageType);

 private:
    epicsTime next_run;
    bool quiet;
    Event done_event;
    size_t value_offset;
    uint64 updates;
    uint64 overruns;
    uint64 last_pulse_id;
    uint64 missing_pulses;

    std::string m_requesterName;

    void checkUpdate(shared_ptr<PVStructure> const &structure);

  };


 
}

#endif //NEDCHANNEL_H





