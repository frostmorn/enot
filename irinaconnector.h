#ifdef GHOST_IRINA
#ifndef IRINA_CONNECTOR_H
#define IRINA_CONNECTOR_H
#define IRINA_HOST "195.88.208.203"
#define IRINA_PORT 4742U
#define IRINA_TIMEOUT 50

#include <thread>
#include <sys/select.h>
#include <string>
class CGHost;
class CIrinaProtocol;
class CIrinaConnector;
class CTCPClient;
class CIncomingIrinaAuthToken;
class CIrinaConnector{
private:
    int nfds;
    fd_set fd;
	fd_set send_fd;

    CGHost *m_GHost;
    std::string token;
    CIrinaProtocol *irina_proto;
    CTCPClient *m_Socket;
    std::thread *irina_thread;
    bool m_Authorized = false;
    
    void Authorize();
    //void Update();
    void IrinaLoop();
public:
    bool GetAuthorized(){ m_Authorized;}
    void Run();
    CIrinaConnector(CGHost *nGHost, std::string &token);
    ~CIrinaConnector();
};

#endif
#endif