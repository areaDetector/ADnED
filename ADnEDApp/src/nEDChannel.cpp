

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

#include "nEDChannel.h"

namespace nEDChannel {

  using std::cout;
  using std::endl;
  using std::string;

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
  
  boolean nEDChannelRequester::waitUntilConnected(double timeOut) 
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
  
  nEDMonitorRequester::nEDMonitorRequester(std::string &requester_name) : MonitorRequester(), m_requesterName(requester_name)
  {
    cout << "nEDMonitorRequester constructor." << endl;
    cout << "m_requesterName: " << m_requesterName << endl;
  }

  nEDMonitorRequester::~nEDMonitorRequester() 
  {
    cout << "nEDMonitorRequester destructor." << endl;
  }

  void nEDMonitorRequester::monitorConnect(Status const & status, MonitorPtr const & monitor, StructureConstPtr const & structure)
  {
    cout << "Monitor connects, " << status << endl;
    if (status.isSuccess())
    {
        // Check the structure by using only the Structure API?
        // Need to navigate the hierarchy, won't get the overall PVStructure offset.
        // Easier: Create temporary PVStructure
        PVStructurePtr pvStructure = getPVDataCreate()->createPVStructure(structure);
        shared_ptr<PVULong> value = pvStructure->getULongField("pulse.value");
        if (! value)
        {
            cout << "No 'pulse.value' ULong" << endl;
            return;
        }
        value_offset = value->getFieldOffset();
        // pvStructure is disposed; keep value_offset to read data from monitor's pvStructure

        monitor->start();
    }
  }

  void nEDMonitorRequester::monitorEvent(MonitorPtr const & monitor)
  {
    shared_ptr<MonitorElement> update;
    while ((update = monitor->poll()))
      {
        // TODO Simulate slow client -> overruns on client side
        // epicsThreadSleep(0.1);

        ++updates;
        //checkUpdate(update->pvStructurePtr);
        // update->changedBitSet indicates which elements have changed.
        // update->overrunBitSet indicates which elements have changed more than once,
        // i.e. we missed one (or more !) updates.
        //if (! update->overrunBitSet->isEmpty())
        //    ++overruns;
        //if (quiet)
        //{
	//epicsTime now(epicsTime::getCurrent());
	//  if (now >= next_run)
	//  {
	      //double received_perc = 100.0 * updates / (updates + missing_pulses);
	      //cout << updates << " updates, "
	      //     << overruns << " overruns, "
	      //     << missing_pulses << " missing pulses, "
	      //     << "received " << fixed << setprecision(1) << received_perc << "%"
	      //     << endl;
	      //overruns = 0;
	      //missing_pulses = 0;
	      //updates = 0;

	//    next_run = now + 10.0;
	//  }
	    //}
	    //else
	    //{
	cout << "Monitor:\n";

            //cout << "Changed: " << *update->changedBitSet.get() << endl;
            //cout << "Overrun: " << *update->overrunBitSet.get() << endl;
	    
	cout << "Updates: " << updates << endl;

            //update->pvStructurePtr->dumpValue(cout);
            //cout << endl;

	monitor->release(update);

	//This doesn't stop anything
	//if (updates==10) {
	//cout << "Stopping monitor" << endl;
	//monitor->stop();
	//break;
	//}

      }
  }

  boolean nEDMonitorRequester::waitUntilDone()
  {
    return done_event.wait();
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
