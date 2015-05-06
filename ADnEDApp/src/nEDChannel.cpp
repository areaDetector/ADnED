

#include <iostream>
#include <string>
#include <stdexcept>
#include "dirent.h"
#include <sys/types.h>
#include <syscall.h> 

//Epics headers
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <iocsh.h>
#include <drvSup.h>
#include <registryFunction.h>

#include "ADnED.h"
#include "nEDChannel.h"

namespace nEDChannel {

  using std::cout;
  using std::endl;
  using std::string;

  using std::tr1::shared_ptr;
  using namespace epics::pvData;
  using namespace epics::pvAccess;

  //ChannelRequester

  nEDChannelRequester::nEDChannelRequester(std::string &requester_name) : ChannelRequester(), m_requesterName(requester_name)
  {
    cout << "nEDChannelRequester constructor." << endl;
    cout << "m_requesterName: " << m_requesterName << endl;
  }

  nEDChannelRequester::~nEDChannelRequester() 
  {
    cout << "nEDChannelRequester destructor." << endl;
  }

  void nEDChannelRequester::channelCreated(const Status& status, Channel::shared_pointer const & channel)
  {
    cout << channel->getChannelName() << " created, " << status << endl;
  }

  void nEDChannelRequester::channelStateChange(Channel::shared_pointer const & channel, Channel::ConnectionState connectionState)
  {
    cout << channel->getChannelName() << " state: "
         << Channel::ConnectionStateNames[connectionState]
         << " (" << connectionState << ")" << endl;
    if (connectionState == Channel::CONNECTED) {
      m_connectEvent.signal();
    }
  }
  
  bool nEDChannelRequester::waitUntilConnected(double timeOut) 
  {
    cout << "Waiting for connection." << endl;
    return m_connectEvent.wait(timeOut);
  }
 
  string nEDChannelRequester::getRequesterName()
  {   
    return m_requesterName; 
  }
 
  void nEDChannelRequester::message(string const &message, MessageType messageType)
  {
    cout << getMessageTypeName(messageType) << ": "
         << m_requesterName << " "
         << message << endl;
  }

  //MonitorRequester
  
  nEDMonitorRequester::nEDMonitorRequester(std::string &requester_name, ADnED *nED, epicsUInt32 channelID) : 
    MonitorRequester(), m_requesterName(requester_name), p_nED(nED), m_channelID(channelID)
  {
    cout << "nEDMonitorRequester constructor." << endl;
    cout << "m_requesterName: " << m_requesterName << endl;
    cout << "p_nED: " << std::hex << p_nED << std::dec << endl;
    cout << "m_channeID: " << m_channelID << endl;
  }

  nEDMonitorRequester::~nEDMonitorRequester() 
  {
    cout << "nEDMonitorRequester destructor." << endl;
  }

  void nEDMonitorRequester::monitorConnect(Status const & status, MonitorPtr const & monitor, StructureConstPtr const & structure)
  {
    cout << "Monitor connects, " << status << endl;
    if (status.isSuccess()) {
      PVStructurePtr pvStructure = getPVDataCreate()->createPVStructure(structure);
      shared_ptr<PVInt> value = pvStructure->getIntField("timeStamp.userTag");
      if (!value) {
        return;
      }
      m_connectEvent.signal();
    }
  }

  bool nEDMonitorRequester::waitUntilConnected(double timeOut) 
  {
    cout << "Waiting for monitor." << endl;
    return m_connectEvent.wait(timeOut);
  }

  void nEDMonitorRequester::monitorEvent(MonitorPtr const & monitor)
  {
    shared_ptr<MonitorElement> update;
    while ((update = monitor->poll())) {
      p_nED->eventHandler(update->pvStructurePtr, m_channelID);
      monitor->release(update);
    }
  }
  
  boolean nEDMonitorRequester::waitUntilDone()
  {
    return m_doneEvent.wait();
  }

  void nEDMonitorRequester::unlisten(MonitorPtr const & monitor)
  {
    cout << "Monitor unlistens" << endl;
  }

  string nEDMonitorRequester::getRequesterName()
  {   
    return m_requesterName; 
  }
 
  void nEDMonitorRequester::message(string const &message, MessageType messageType)
  {
    cout << getMessageTypeName(messageType) << ": "
         << m_requesterName << " "
         << message << endl;
  }


} // namespace nEDChannel
