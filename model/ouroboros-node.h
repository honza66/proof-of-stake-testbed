#ifndef PROJECT_OUROBOROS_NODE_H
#define PROJECT_OUROBOROS_NODE_H

#include "blockchain-node.h"
#include "ns3/event-id.h"
#include "ouroboros-helper.h"

namespace ns3 {

    class OuroborosNodeApp : public BlockChainNodeApp {
    private:
        EventId sendingSeedNextEvent;
        EventId newSlotNextEvent;
        Block* createdBlock;
        std::vector<Ipv4Address> nodesAddresses;
        std::vector<std::vector<int>> receivedSeeds;   //vector of epochs (vector indexed by nodes contain received epoch num)
    protected:
        OuroborosHelper *nodeHelper;

        int CreateSecret();

        int GetSlotNumber();

        int GetEpochNumber();

        int GetSlotLeader(int slotNumber, int epochNumber);

        void StartApplication(void);

        void StopApplication(void);

        bool HandleCustomRead(Ptr <Packet> packet, Address from, std::string receivedData) override;

        bool SaveEpochNum(int epochNum, int value, int nodeId);

        /**
         * Handle new received transaction
         * @param receivedData
         */
        void ReceiveNewTransaction(rapidjson::Document *message) override ;

        bool IsIamLeader();

        void StartNewSlot();

        void FinishActualSlot();

    public:
        OuroborosNodeApp(OuroborosHelper *nodeHelper);

        void SendEpochSeed();

        void ReceiveEpochSeed(rapidjson::Document *message);
    };
}

#endif //PROJECT_OUROBOROS_NODE_H
