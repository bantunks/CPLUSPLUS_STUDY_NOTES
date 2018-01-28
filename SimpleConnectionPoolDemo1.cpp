#include <iostream>
#include <exception>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
//#include "ConnectionPool.hpp"
#define INIT_POOL_SIZE 10
#define MAX_POOL_SIZE 100
#define NEW_CONNECTION_INCREMENT 10

using namespace std;
typedef enum state
{
  FREE = 0,
  ROGUE,
  ALLOCATED
}CONN_STATE;

typedef class endPoint
{
  char service[10];
  int port;

  public:
    endPoint()
    {
    }
    endPoint(char* host, int portNum)
    {
#ifdef DEBUG
        cout << "Inside endPoint Parameterized Constructor" << endl;
#endif
      strncpy(service, host, strlen(host));
      port = portNum;
    }
    char* getService()
    {
      return service;
    }
    int getPort()
    {
      return port;
    }
    void setService(char* service)
    {
      strncpy(this->service, service, strlen(service));
    }
    void setPort(int port)
    {
      this->port = port;
    }
    ~endPoint()
    {
#ifdef DEBUG
        cout << "Inside endPoint Destructor" << endl;
#endif
    }
}EndPoint;

typedef struct Connection
{
  EndPoint **endPointPair;
  CONN_STATE state;

  int id;
  time_t connectionStartTime;

  Connection()
  {
  }

  Connection(int num):id(num)
  {
#ifdef DEBUG
    cout << "Inside Connection Constructor" << endl;
#endif
  }

  int getConnectionId()
  {
    return id;
  }

  void resetConnectionStartTime()
  {
    connectionStartTime = 0;
  }

  void setState(CONN_STATE state)
  {
    this->state = state;
  }

  void addEndPointsToConnection(EndPoint* endPointSource, EndPoint* endPointDest) //throws Exception
  {
    // Add logic to check for rogue input arguments
#ifdef DEBUG
      cout << "Inside addEndPointsToConnection" << endl;
#endif
    endPointPair = new EndPoint*[2];
    endPointPair[0] = new EndPoint(endPointSource->getService(), endPointSource->getPort());
    endPointPair[1] = new EndPoint(endPointDest->getService(), endPointDest->getPort());

#ifdef DEBUG
    cout << "endPointPair" << endPointPair << endl;
    cout << "endPointPair[0]" << endPointPair[0] << endl;
    cout << "endPointPair[1]" << endPointPair[1] << endl;
#endif

    this->state = ALLOCATED;
    //current timestamp
    this->connectionStartTime = time(0);
  }

  void destructConnection()
  {
    // Call EndPoint destrcutor to free the allocated endpoints
    // Retain the identifier for the connection, just mark its state as free
    releaseEndPoints();
    setState(FREE); // Can move this to Connection constructor
    id = -1; // This marks the id as invalid
    resetConnectionStartTime();
  }

  void releaseEndPoints()
  {
#ifdef DEBUG
    cout << "Inside releaseEndPoints" << endl;
#endif
    delete endPointPair[0];
    endPointPair[0] = NULL;
    delete endPointPair[1];
    endPointPair[1] = NULL;
    delete [] endPointPair;
#ifdef DEBUG
    cout << "Leaving releaseEndPoints" << endl;
#endif
  }

  ~Connection()
  {
#ifdef DEBUG
    cout << "Inside Connection Destructor" << endl;
#endif
    destructConnection();
#ifdef DEBUG
    cout << "Inside Connection Destructor" << endl;
#endif
  }
}Connection;

class ConnectionPool
{
  private:
    //Connection *connPool[MAX_POOL_SIZE];
    Connection **connPool;
    // Any static variables must be guarded with appropriate IPC mechanism, e.g. Mutex
    static atomic_int curAllocated;
    static atomic_int curAssigned;
    // We do not need a pool level mutex if we are guarding the counters correctly
    //pthread_mutex_t poolMutex;
    pthread_mutex_t curAllocatedMutex;
    pthread_mutex_t curAssignedMutex;

  public:

    ConnectionPool()
    {
      // Better to use ATOMIC datatypes instead of the mutexes if the variables are simple integers
      //initializeConnectionPool(); // Move this to ConnectionPoolManager class
      connPool = new Connection*[MAX_POOL_SIZE];
    }

    void addNewConnectionToPool()
    {
      #ifdef DEBUG
        cout << "Inside addNewConnectionToPool" << endl;
        cout << "ConnectionPool::Allocated" << ConnectionPool::curAllocated << endl;
      #endif
      try {
          ConnectionPool::curAllocated++;
          connPool[ConnectionPool::curAllocated] = new Connection(ConnectionPool::curAllocated);
      } catch(exception& ex) {
        //throw OutOfMemoryException;
      }
    }

    void addNextBatchNewConnections()
    {
      for(int i = 0; i < NEW_CONNECTION_INCREMENT; i++) {
        addNewConnectionToPool();
      }
    }

    // Do we need mutex here ???
    Connection* createAndReturnNextConnection(EndPoint* endPointsSource, EndPoint* endPointsDest)
    {
      #ifdef DEBUG
        cout << "Inside createAndReturnNextConnection" << endl;
        cout << "ConnectionPool::curAssigned" << ConnectionPool::curAssigned << endl;
      #endif

      pthread_mutex_lock(&curAssignedMutex);
      ConnectionPool::curAssigned++;
      // Since curAssigned is static, all threads share a single copy and hence mutex alone is not sufficient to protect the coutner
      // For such cases, we need to use a local temp variable to save the current thread's version of static variable curAssigned
      // this temp variable is local to each thread and hence the correct Conection object would be returned for each thread
      int temp = curAssigned;
      pthread_mutex_unlock(&curAssignedMutex);
      connPool[temp]->addEndPointsToConnection(endPointsSource, endPointsDest);
      return (connPool[temp]);
    }

    Connection* allocConnection(EndPoint* endPointSource, EndPoint* endPointDest)
    {
      pthread_mutex_lock(&curAllocatedMutex);
      pthread_mutex_lock(&curAssignedMutex);
      if (ConnectionPool::curAssigned == (MAX_POOL_SIZE - 1)) {
        // Check for any rogue connections and kill/release them forcibly
        // if no rogue connections found, throw exception OutOfConnection
        // if (checkRogueConnections() != -1)
          // throw OUT OF CONNECTIONS Exception
      } else if (ConnectionPool::curAssigned == ConnectionPool::curAllocated) {
          addNextBatchNewConnections();
      }
      pthread_mutex_unlock(&curAssignedMutex);
      pthread_mutex_unlock(&curAllocatedMutex);
      return createAndReturnNextConnection(endPointSource, endPointDest);
    }

    //void releaseConnection(Connection** connection)
    void releaseConnection(Connection** connection)
    {
//#ifdef DEBUG
      cout << "Inside releaseConnection" << endl;
//#endif
      // releaseConnectionToPool(connection->getId());
      // Call EndPoint destrcutor to free the allocated endpoints
      // Retain the identifier for the connection, just mark its state as free
      (*connection)->releaseEndPoints();
      (*connection)->setState(FREE); // Can move this to Connection constructor
      (*connection)->resetConnectionStartTime();
    }

    // atomic types are neither copyable nor movable, hence we cannot write setters and getters functions to initialize or assign to them
    /*
    atomic_int getCurAllocated()
    {
      return ConnectionPool::curAllocated;
    }

    atomic_int getCurAssigned()
    {
      return ConnectionPool::curAssigned;
    }


    void setCurAllocated(atomic_int val)
    {
      ConnectionPool::curAllocated = val;
    }

    void setCurAssigned(atomic_int val)
    {
      ConnectionPool::curAssigned = val;
    }
    */

//// Rough Logic for detecting rogue connections
/********************************************************************************************************************
    int checkRogueConnection()
    {
      for (int i = 0; i < MAX_POOL_SIZE; i++) {
        time_t now = time(0);
        // if the time difference is more than 4 hours, check for progress
        // check for progress logic follows --
        // 1. Poll the host service to check if there is any progress on ths connection
        // 2. If the service returns this connection as ROGUE, mark the connection as ROGUE and return TRUE
        // 3. We may want to return an array of ROGUE connections in future
        if(time_diff(connPool[i]->connectionStartTime, now) {
          checkWithService(connPool[i].endPointPair);
          connPool[i]->state = ROGUE;
          return i;
        }
      }
      return -1;
    }
********************************************************************************************************************/


    ~ConnectionPool()
    {
      cout << "ConnectionPool Cleaning Up" << endl;
      cout << "Number of Allocated Connections in Pool: " << ConnectionPool::curAllocated << endl;
      cout << "Number of Assigned(active) Connections in Pool: " << ConnectionPool::curAssigned << endl;
      pthread_mutex_lock(&curAllocatedMutex);
      pthread_mutex_lock(&curAssignedMutex);
      int i = 0;
      for (; i <= ConnectionPool::curAllocated; i++) { // curAllocated and curAssigned are initialized to -1, hence we need to use <=
        delete connPool[i];
      }

      delete [] connPool;
      ConnectionPool::curAssigned = -1;
      ConnectionPool::curAllocated = -1;
      pthread_mutex_unlock(&curAllocatedMutex);
      pthread_mutex_unlock(&curAssignedMutex);
      pthread_mutex_destroy(&curAllocatedMutex);
      pthread_mutex_destroy(&curAssignedMutex);
    }

};

// Understand why we cannot do "atomic_int ConnectionPool::curAllocated = -1"
// This is because the overloaded assignment copy operator is not defined for atomic_int
atomic_int ConnectionPool::curAllocated = {-1};
atomic_int ConnectionPool::curAssigned = {-1};

class ConnectionPoolMgr
{
  private:
    ConnectionPool *connPool;
  public:
    ConnectionPoolMgr()
    {
      initConnectionPool();
    }
    void initConnectionPool()
    {
      connPool = new ConnectionPool();
    }
    Connection* allocConnection(EndPoint* src, EndPoint* dst)
    {
      return connPool->allocConnection(src, dst);
    }
    ~ConnectionPoolMgr()
    {
#ifdef DEBUG
      cout << "Inside ConnectionPoolMgr Destructor" << endl;
#endif
      delete connPool;
      connPool = NULL;
    }
};

int main()
{
  cout << "Inside Main()" << endl;
  char srcService[10] = "hello";
  char dstService[10] = "world";
  EndPoint *src = new EndPoint(srcService, 7777);
  EndPoint *dst = new EndPoint(dstService, 8888);
  ConnectionPoolMgr mgr;
  Connection* newConnection[80];
  int i = 0;
  while (i < 80) {
    newConnection[i] = mgr.allocConnection(src, dst);
    cout << "newConnection[i]: " << newConnection[i] << endl;
    i++;
  }
  delete src;
  delete dst;
  return 0;
}

