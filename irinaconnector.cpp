#ifdef GHOST_IRINA
#include "irinaconnector.h"
#include "irinaprotocol.h"
#include "ghost.h"
#include "socket.h"
#include "util.h"
#include <string>
CIrinaConnector::CIrinaConnector(CGHost *nGHost, std::string &token)
{
    FD_ZERO(&fd);
    FD_ZERO(&send_fd);
    this->irina_proto = new CIrinaProtocol();
    this->m_Socket = new CTCPClient();
    this->m_GHost = nGHost;
    this->token = token;
    m_Socket->SetFD(&fd, &send_fd, &nfds);
    CONSOLE_Print("[IRINA] Allocated new CIrinaConnector");
    this->m_Socket->Connect(this->m_GHost->m_BindAddress, IRINA_HOST, IRINA_PORT);
}

CIrinaConnector::~CIrinaConnector()
{
}

void CIrinaConnector::Authorize()
{
    if (m_Socket->GetConnected())
    {
        m_Socket->PutBytes(this->irina_proto->IRINA_CreateAuthToken(new CIncomingIrinaAuthToken(this->token)) + "\n");
        this->m_Authorized = true;
        CONSOLE_Print("[IRINA] Irina authorized");
    }
    else
    {
        this->m_Authorized = false;
    }
}
void CIrinaConnector::IrinaLoop()
{
    // Handle irina connection

    while (1)
    {
        if (!m_Socket->GetConnecting() && !m_Socket->GetConnected())
        {
            this->m_Authorized = false;
        }

        if (m_Socket->GetConnecting() && !m_Socket->GetConnected())
        {
            // Attempting connect to irina host
            if (m_Socket->CheckConnect())
            {
                CONSOLE_Print("[IRINA] IrinaConnector connected!");
                this->Authorize();
            }
            else
            {
                m_Authorized = false;
                CONSOLE_Print("[IRINA] Something bad actually happened while trying to connect");
            }
        }

        // Handling irina connection
        if (m_Socket->GetConnected())
        {
            // Recieve/Send what we need
            this->m_Socket->DoRecv(&fd);
            this->m_Socket->DoSend(&send_fd);
 
            // Send pong message every IRINA_TIMEOUT seconds if we don't want to disconnect
            if (GetTime() - m_Socket->GetLastSend() > IRINA_TIMEOUT)
            {
                CONSOLE_Print("[IRINA] Sending pong message to irina due to timeout");
                // m_Socket->PutBytes("{}\n"); // pong proto
                this->Authorize();
            }

            std::string *RecvBuffer = m_Socket->GetBytes();
            if (!RecvBuffer->empty())
            {
                CONSOLE_Print("[IRINA]Irina sent bytes:" + *RecvBuffer);
                m_Socket->Reset();
            }
        }
        // Handle socket errors
        if (m_Socket->HasError())
        {
            m_Authorized = false;
            CONSOLE_Print("[IRINA] Connector failed with error " + m_Socket->GetError());
            return;
        }

        // Sleep to not fall into shit CPU overheat;
        MILLISLEEP(100);
    }
}
void CIrinaConnector::Run()
{
    // Runing irina thread handler
    CONSOLE_Print("[IRINA] Runing CIrinaConnector::IrinaLoop in a separate thread");
    this->irina_thread = new std::thread(&CIrinaConnector::IrinaLoop, this);
}
#endif