#ifndef DASHBOARDSESSION_H
#define DASHBOARDSESSION_H


#include "frame_transport.hpp"

/// FrameTransport used by the dashboard client.
class DashboardSession : public FrameTransport
{
public:
    explicit DashboardSession(std::unique_ptr<ISocket> socket);

    // Not used: the dashboard is not an agent, it has no OS info to register.
    const OsInfoPayload &getAgentInfo() const override;
    void setAgentInfo(const OsInfoPayload &info) override;
};

#endif // DASHBOARDSESSION_H
