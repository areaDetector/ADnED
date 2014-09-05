

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
    if (connectionState == Channel::CONNECTED)
      m_connectEvent.signal();
  }
  
  boolean nEDChannelRequester::waitUntilConnected(double timeOut) 
  {
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

  
}
