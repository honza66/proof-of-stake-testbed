

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/log.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "blockchain.h"
#include "messages.h"
#include "constants.h"
#include <vector>
#include "../../network/utils/ipv4-address.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"

namespace ns3 {
    NS_LOG_COMPONENT_DEFINE ("BlockChain");

    class Ipv4Address;

    /*------------ Transaction ---------------*/

    Transaction::Transaction(int senderId, int receiverId) {
        this->id = rand(); //TODO: make it clever and better (long int)
        this->senderId = senderId;
        this->receiverId = receiverId;
    }

    Transaction::Transaction(long int id, int senderId, int receiverId) {
        this->id = id;
        this->senderId = senderId;
        this->receiverId = receiverId;
    }

    int Transaction::GetReceiverId() const {
        return this->receiverId;
    }

    int Transaction::GetSenderId() const {
        return this->senderId;
    }

    int Transaction::GetId() const {
        return this->id;
    }

    rapidjson::Document Transaction::ToJSON() {
        const char *json = "{\"type\":\"1\", \"id\":\"1\",\"senderId\":1,\"receiverId\":1}";
        rapidjson::Document message;
        message.Parse(json);
        message["id"].SetInt(this->id);
        message["type"].SetInt(NEW_TRANSACTION);
        message["senderId"].SetInt(this->senderId);
        message["receiverId"].SetInt(this->receiverId);
        return message;
    }

    Transaction Transaction::FromJSON(rapidjson::Document *document) {
        Transaction transaction((*document)["id"].GetInt(), (*document)["senderId"].GetInt(), (*document)["receiverId"].GetInt());
        return transaction;
    }

    /*------------ BLOCK ---------------*/

    Block::Block(int blockHeight, int validatorId, Block *previousBlock, double timeCreated,
                 double timeReceived,
                 Ipv4Address receivedFrom) {
        this->blockHeight = blockHeight;
        this->validatorId = validatorId;
        this->previousBlock = previousBlock;
        this->timeCreated = timeCreated;
        this->timeReceived = timeReceived;
        this->receivedFrom = receivedFrom;
    }


    int Block::GetBlockHeight() const {
        return blockHeight;
    }

    int Block::GetBlockSize(){
        return this->transactions.size();
    }

    bool Block::IsBlockFull(){
        return this->GetBlockSize() >= constants.maxTransactionsPerBlock;
    }

    int Block::GetValidatorId() const {
        return validatorId;
    }

    Block *Block::GetPreviousBlock() const {
        return previousBlock;
    }

    double Block::GetTimeCreated() const {
        return timeCreated;
    }

    double Block::GetTimeReceived() const {
        return timeReceived;
    }

//    IPv4Address Block::GetReceivedFrom() {
//        return receivedFrom;
//    }

    void Block::AddTransaction(Transaction transaction){
        if(this->IsBlockFull()){
            //TODO exception
            return;
        }
        this->transactions.push_back(transaction);
    }

    std::vector <Transaction> Block::GetTransactions(){
        return this->transactions;
    }

    std::vector <Transaction> Block::GetTransactionsByReceiver(int receiverId){
        std::vector <Transaction> results;
        for(auto const& trans: this->transactions) {
            if(trans.GetReceiverId() == receiverId){
                results.push_back(trans);
            }
        }
        return results;
    }

    std::vector <Transaction> Block::GetTransactionsBySender(int senderId){
        std::vector <Transaction> results;
        for(auto const& trans: this->transactions) {
            if(trans.GetReceiverId() == senderId){
                results.push_back(trans);
            }
        }
        return results;
    }

    bool operator==(const Block &block1, const Block &block2) {
        if(block1.GetBlockHeight() == block2.GetBlockHeight() && block1.GetValidatorId() == block2.GetValidatorId()){
            return true;
        }
        return false;
    }

    rapidjson::Document Block::ToJSON() {
        const char *json = "{\"type\":\"1\", \"blockHeight\":1, \"blockSize\":1,\"validatorId\":1,\"timeCreated\":1}";
        rapidjson::Document message;
        message.Parse(json);
        message["type"].SetInt(NEW_BLOCK);
        message["blockHeight"].SetInt(this->blockHeight);
        message["blockSize"].SetInt(this->GetBlockSize());
        message["validatorId"].SetInt(this->validatorId);
        message["timeCreated"].SetDouble(this->timeCreated);
//        rapidjson::Document::AllocatorType& allocator = message.GetAllocator();
//        for(auto const& trans: this->transactions) {
//            rapidjson::Document transDoc = trans.ToJSON();
//            message["transactions"].push_back(trans);
//        }
        return message;
    }

    Block Block::FromJSON(rapidjson::Document *document) {

    }

    /*------------ BLOCKChain ---------------*/


    BlockChain::BlockChain() {
        Block* block = new Block(0, 0, nullptr, 0, 0, Ipv4Address("0.0.0.0"));
        this->AddBlock(block);
    }

    int BlockChain::GetTotalCountOfBlocks(){
        return this->totalCountOfBlocks;
    }

    Block *BlockChain::GetTopBlock() {
        return this->blocks[this->blocks.size()-1][0];
    }

    int BlockChain::GetBlockchainHeight() {
        return this->GetTopBlock()->GetBlockHeight();
    }

    bool BlockChain::HasBlock(Block *block) {
        if (block->GetBlockHeight() > this->GetBlockchainHeight()) {
            return false;
        }
        auto column = this->blocks[block->GetBlockHeight()];
        for (auto &value: column) {
            if (block == value) {
                return true;
            }
        }
        return false;
    }

    void BlockChain::AddBlock(Block *block) {
        if (this->blocks.size() == 0) {
            // add genesis block
            std::vector<Block*> newColumn;
            newColumn.push_back(block);
            this->blocks.push_back(newColumn);
        } else if (block->GetBlockHeight() > this->GetBlockchainHeight()) {
            //add block to the end of rows
            int emptyColums = this->GetBlockchainHeight() - block->GetBlockHeight() - 1;
            for (int i = 0; i < emptyColums; i++) {
                std::vector<Block*> newColumn;
                this->blocks.push_back(newColumn);
            }
            std::vector<Block*> newColumn;
            newColumn.push_back(block);
            this->blocks.push_back(newColumn);
        } else {
            // add to existing column
            this->blocks[block->GetBlockHeight()].push_back(block);
        }
        this->totalCountOfBlocks++;
    }


}

