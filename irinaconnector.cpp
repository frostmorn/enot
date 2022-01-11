#ifdef GHOST_IRINA
#include "irinaconnector.h"
#include "irinaprotocol.h"
#include "ghost.h"
#include "socket.h"
#include "util.h"
#include <string>
CIrinaConnector::CIrinaConnector(CGHost *nGHost, std::string &nToken)
{

    m_IrinaProto = new CIrinaProtocol();
    m_Socket = new CTCPClient();
    m_GHost = nGHost;
    m_Token = nToken;

    FD_ZERO(&fd);
    FD_ZERO(&send_fd);

    m_Socket->SetFD(&fd, &send_fd, &nfds);

    m_Socket->Connect(m_GHost->m_BindAddress, IRINA_HOST, IRINA_PORT);

    CONSOLE_Print("[IRINA] Allocated new CIrinaConnector");
}

CIrinaConnector::~CIrinaConnector()
{
}

void CIrinaConnector::Authorize()
{
    if (m_Socket->GetConnected())
    {
        m_Socket->PutBytes(m_IrinaProto->IRINA_CreateAuthToken(new CIncomingIrinaAuthToken(m_Token)) + "\n");
        m_Authorized = true;
        CONSOLE_Print("[IRINA] Irina authorized");
    }
    else
    {
        m_Authorized = false;
    }
}
void CIrinaConnector::IrinaLoop()
{
    // Handle irina connection

    while (1)
    {
        if (!m_Socket->GetConnecting() && !m_Socket->GetConnected())
        {
            m_Authorized = false;
        }

        if (m_Socket->GetConnecting() && !m_Socket->GetConnected())
        {
            // Attempting connect to irina host
            if (m_Socket->CheckConnect())
            {
                CONSOLE_Print("[IRINA] IrinaConnector connected!");
                Authorize();
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
            m_Socket->DoRecv(&fd);
            m_Socket->DoSend(&send_fd);

            // Send pong message every IRINA_TIMEOUT seconds if we don't want to disconnect
            if (GetTime() - m_Socket->GetLastSend() > IRINA_TIMEOUT)
            {
                CONSOLE_Print("[IRINA] Sending pong message to irina due to timeout");
                // m_Socket->PutBytes("{}\n"); // pong proto
                Authorize();
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
    m_IrinaThread = new std::thread(&CIrinaConnector::IrinaLoop, this);
}
#endif