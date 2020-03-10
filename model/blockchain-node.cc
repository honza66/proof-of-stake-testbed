#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/event-id.h"
#include "blockchain-node.h"
#include "../utils/rsa.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE ("BlockChainNodeApp");
    NS_OBJECT_ENSURE_REGISTERED (BlockChainNodeApp);

    BlockChainNodeApp::BlockChainNodeApp(Ipv4InterfaceContainer netContainer) {
        this->listenSocket = 0;
        this->broadcastSocket = 0;
        this->keys = generate_keys();
        this->netContainer = netContainer;
    }

    BlockChainNodeApp::BlockChainNodeApp() {
        this->listenSocket = 0;
        this->keys = generate_keys();
    }

    TypeId BlockChainNodeApp::GetTypeId() {
        static TypeId tid = TypeId("ns3::BlockChainNodeApp")
                .SetParent<Application>()
                .SetGroupName("Applications")
                .AddConstructor<BlockChainNodeApp>();
        return tid;
    }

    Ptr <Socket> BlockChainNodeApp::GetListenPort() const {
        return this->listenSocket;
    };

    void BlockChainNodeApp::StartApplication() {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("Starting App");

        // listen Socket
        if (!this->listenSocket) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");     //udp
            this->listenSocket = Socket::CreateSocket(GetNode(), tid);
            InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 655);
            this->listenSocket->Bind(local);
            this->listenSocket->Listen();
            this->listenSocket->ShutdownSend();
            if (addressUtils::IsMulticast(this->multicastLocal)) {
                Ptr <UdpSocket> udpSocket = DynamicCast<UdpSocket>(this->listenSocket);
                if (udpSocket) {
                    // equivalent to setsockopt (MCAST_JOIN_GROUP)
                    udpSocket->MulticastJoinGroup(0, this->multicastLocal);
                } else {
                    NS_FATAL_ERROR("Error: Failed to join multicast group");
                }
            }
        }
        this->listenSocket->SetRecvCallback(MakeCallback(&BlockChainNodeApp::HandleRead, this));
        this->listenSocket->SetAcceptCallback(MakeNullCallback < bool, Ptr < Socket > ,const Address &> (),
                MakeCallback(&BlockChainNodeApp::HandleConnectionAccept, this));
        this->listenSocket->SetCloseCallbacks(
                MakeCallback(&BlockChainNodeApp::HandleConnectionEnd, this),
                MakeCallback(&BlockChainNodeApp::HandleConnectionEnd, this)
        );
        this->listenSocket->SetAllowBroadcast (true);
        this->nextEvent = Simulator::Schedule(Seconds(0.0), &BlockChainNodeApp::Send, this);


        //broadcast socket
        if (!this->broadcastSocket) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            this->broadcastSocket = Socket::CreateSocket (GetNode (), tid);
            static const InetSocketAddress broadcastAddress = InetSocketAddress(Ipv4Address("255.255.255.255"), 655);
            this->broadcastSocket->Connect (broadcastAddress);
            this->broadcastSocket->SetAllowBroadcast (true);
            this->broadcastSocket->ShutdownRecv ();
        }


        //socket to all nodes
        for (std::vector<Ipv4Address>::const_iterator i = this->nodesAddresses.begin(); i != this->nodesAddresses.end(); ++i) {
            TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
            this->nodesSockets[*i] = Socket::CreateSocket(GetNode(), tid);
            this->nodesSockets[*i]->Connect(InetSocketAddress(*i, 655));
        }

    }

    void BlockChainNodeApp::StopApplication() {
        NS_LOG_FUNCTION(this);

        if (this->listenSocket != 0) {
            this->listenSocket->Close();
            this->listenSocket->SetRecvCallback(MakeNullCallback < void, Ptr < Socket > > ());
        }
        Simulator::Cancel(this->nextEvent);
    }

    void BlockChainNodeApp::SetNodesAddresses(std::vector <Ipv4Address> &addresses) {
        this->nodesAddresses = addresses;
    }

    void BlockChainNodeApp::HandleRead(Ptr <Socket> socket) {
        NS_LOG_FUNCTION(this << socket);

        Ptr <Packet> packet;
        Address from;

        while ((packet = socket->RecvFrom(from))) {
            double receiveTimeSeconds = Simulator::Now().GetSeconds();
            if(Inet6SocketAddress::IsMatchingType(from)){

            } else if(InetSocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time " << receiveTimeSeconds  << "s server received " << packet->GetSize()
                                       << " bytes from " <<
                                       InetSocketAddress::ConvertFrom(from).GetIpv4() << " port " <<
                                       InetSocketAddress::ConvertFrom(from).GetPort() << " data: " << packet->ToString());
            } else {
                NS_FATAL_ERROR("Error: Received unsupported bytes");
            }

//            packet->RemoveAllPacketTags();
//            packet->RemoveAllByteTags();

            //response
//            NS_LOG_LOGIC("Echoing packet");
//            socket->SendTo(packet, 0, from);

        }
    }

    void BlockChainNodeApp::HandleConnectionAccept(Ptr<Socket> socket, const Address& address){
        NS_LOG_FUNCTION(this);
        double timeSeconds = Simulator::Now().GetSeconds();
        NS_LOG_INFO("At time " << timeSeconds  << "s node " << GetNode()->GetId() << " accepting conenction");
    }

    void BlockChainNodeApp::HandleConnectionEnd(Ptr<Socket> socket){
        NS_LOG_FUNCTION(this);
        double timeSeconds = Simulator::Now().GetSeconds();
        NS_LOG_INFO("At time " << timeSeconds  << "s node " << GetNode()->GetId() << " connection end");
    }

    void BlockChainNodeApp::ReceiveBlock(const Block &block){

    }

    void BlockChainNodeApp::ReceiveBlocks(std::vector <Block> &blocks){

    }

    void BlockChainNodeApp::ScheduleSend (Time dt) {
        NS_LOG_FUNCTION (this << dt);
        this->nextEvent = Simulator::Schedule (dt, &BlockChainNodeApp::Send, this);
    }

    void BlockChainNodeApp::Send() {
        NS_LOG_FUNCTION(this);
        NS_LOG_INFO("sending");

        std::string info = "ahoj";
        rapidjson::Document message;
        message.Parse(info.c_str());
        this->SendMessage(message, this->broadcastSocket);

        ScheduleSend(Seconds (5.0));
    }

    void BlockChainNodeApp::SendMessage(rapidjson::Document &message, Ptr<Socket> outgoingSocket) {
        NS_LOG_FUNCTION(this);

        const uint8_t delimiter[] = "#";
        rapidjson::StringBuffer buffer;

        rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);

        message.Accept(writer);
        double timeSeconds = Simulator::Now().GetSeconds();
//        NS_LOG_INFO("At time " << timeSeconds  << "s node " << GetNode()->GetId()
//                            << " and sent a " << message['type']
//                            << " message: T" /*<< buffer.GetString()*/);

        outgoingSocket->Send(reinterpret_cast<const uint8_t *>(buffer.GetString()), buffer.GetSize(), 0);
        outgoingSocket->Send(delimiter, 1, 0);
    }

    void BlockChainNodeApp::SendMessage(rapidjson::Document &message, Address &outgoingAddress) {
        NS_LOG_FUNCTION(this);

        const uint8_t delimiter[] = "#";
        rapidjson::StringBuffer buffer;
        rapidjson::Writer <rapidjson::StringBuffer> writer(buffer);

        message.Accept(writer);
        double timeSeconds = Simulator::Now().GetSeconds();
//        NS_LOG_INFO("At time " << timeSeconds  << "s node " << GetNode()->GetId()
//                               << " and sent a " << message['type']
//                               << " message: T" << buffer.GetString());

        Ipv4Address address = InetSocketAddress::ConvertFrom(outgoingAddress).GetIpv4();
        auto it = this->nodesSockets.find(address);

        if(it == this->nodesSockets.end()){
            NS_FATAL_ERROR("Node address not found");
        }

        this->nodesSockets[address]->Send(reinterpret_cast<const uint8_t *>(buffer.GetString()), buffer.GetSize(), 0);
        this->nodesSockets[address]->Send(delimiter, 1, 0);
    }
}
