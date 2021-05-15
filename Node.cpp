#include "Include/Node.hpp"
#include "Include/StakePool.hpp"
#include "Include/GenesisBlock.hpp"
#include <stdlib.h> 
#include <time.h>
#include <QCryptographicHash>
#include <qdebug.h>

using namespace std;

class requireMinimalStake: public std::exception{
  virtual const char* what() const throw()
  {
    return "The minimal stake is not respected, cannot create a new node";
  }
}rms;

NodeClass::NodeClass(User* m_owner,QList<Block*> m_blockChain,QList <Transaction> m_ledger, int m_stake):owner(m_owner),
blockChain(m_blockChain),ledger(m_ledger){
  
  try{
    if(m_stake < MINIMAL_STAKE){
      throw rms;
    }
  }
  catch (exception& e)
  {
    cout << e.what() << '\n';
  }
  stake = m_stake;
  online = true;
  isSlotLeader = false;

}

NodeClass NodeClass::electSlotLeader()
{
  GenesisBlock& geBlock = dynamic_cast<GenesisBlock&> (*blockChain.first());
  QListIterator<std::pair<NodeClass,int>> i(geBlock.getStakers());
  int sum=0;

  srand (time(NULL));

  int totalStake=0;
  while(i.hasNext()){
    totalStake += i.next().second;
  }

  int nextSlotLeader = rand() % (totalStake + 1 - 0) + 0;

  i.toFront();
  while(i.hasNext())
  { 
    std::pair<NodeClass,int> next = i.next();
    sum += next.second;
    if(nextSlotLeader < sum ){
      this->isSlotLeader = false;
      next.first.isSlotLeader = true;
      return next.first;
    }
  }
}

bool NodeClass::slotLeaderVerification()
{
  if(isSlotLeader){
    QList<Transaction>::Iterator i;
    for(i=(ledger.begin()+ledger.size())-TRANSACTION_MAX; i != ledger.end(); ++i){
        if(!verifySignature(i->getSender(),*i,i->getSignature())){
          qDebug()<< "Slot leader returned : Wrong Signature";
           return false;
        }
        if(i->getSender().useableStakes < i->getAmount()){
          qDebug()<< "Slot leader returned: User doesn't own enough money to perform this transaction";
          return false;
        }
    }
  }
  else qDebug()<< "Must be the slot leader";

  return true;
}

bool NodeClass::createBlock(){

    if(isSlotLeader == true && slotLeaderVerification()){
      QList<Transaction> blockTransaction;
      QList<Transaction>::iterator i;
      int totalFees=0;
      for(i=(ledger.begin()+ledger.size())-TRANSACTION_MAX;i!= ledger.end();++i){
        blockTransaction.append(*i);
        totalFees += i->getFees();
      }
        Block newBlock((*blockChain.last()).getHash(),signBlock(),computeLastBlockHash(),blockTransaction,
        (*blockChain.last()).getPositionx(),(*blockChain.last()).getPositiony());
        blockChain.append(&newBlock);
        stake += totalFees;
        return true;
    }
    else{
      qDebug()<<"Node not allow to perform such operation or unvalid block";
      return false;
    }
}

void NodeClass::execTransaction(User sender, User receiver, int amount)
{
  sender.useableStakes -= amount;
  receiver.useableStakes += amount;
}

bool NodeClass::verifySignature( User m_user,Transaction t, long long encryptedHash){

  QString s = s.fromStdString(t.toString());
  QByteArray hash = QCryptographicHash::hash(s.toLocal8Bit(),QCryptographicHash::Sha256);
  std::string hashString = hash.toHex(0).toStdString();
  long long ll = std::stoll(hashString);
  long long decryptedHash = decrypt(encryptedHash,m_user.publicKey);
  
  return ll == decryptedHash; 
}

QByteArray NodeClass::computeLastBlockHash(){
    
    QString transactionSeed;

    //Compute ledger Hash
    QList<Transaction>::iterator i;
    for(i=(ledger.begin()+ledger.size())-10;i!= ledger.end();++i){
        transactionSeed += transactionSeed.fromStdString(i->toString());
    }
    return QCryptographicHash::hash(transactionSeed.toLocal8Bit(),QCryptographicHash::Sha256);

}

long long NodeClass::signBlock(){

  if(isSlotLeader){
  QByteArray hash = computeLastBlockHash();
  std::string hashString = hash.toHex(0).toStdString();
  long long ll = std::stoll(hashString);
  return encrypt(ll,owner->publicKey,owner->getPrivateKey());
  }
  else qDebug()<< "The node must be the slot leader to sign a block";
}
