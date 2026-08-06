#include "dashboardsession.h"

DashboardSession::DashboardSession(std::unique_ptr<ISocket> socket)
    : FrameTransport(std::move(socket))
{
}


// The dashboard has no OS info. We return the inherited (empty) member because
// the signature requires a reference to something that outlives the call.
const OsInfoPayload &DashboardSession::getAgentInfo() const { 
        return agentInfo_; 
}

void DashboardSession::setAgentInfo(const OsInfoPayload &) { }