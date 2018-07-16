



#ifndef ThreadPool_HPP
#define ThreadPool_HPP


#include "Thread.hpp"


#include "allegro5/allegro.h"


#include <unordered_set>
#include <deque>
#include <map>


/// Signals to send to MasterThreadProcess
extern const unsigned int STOPMSG;/// Pauses the work
extern const unsigned int STARTMSG;/// Unpauses the work
extern const unsigned int KILLMSG;/// Kills the master thread
extern const unsigned int CONTINUEMSG;/// Way to bypass the event queue and make the master thread loop back to enqueueing jobs

/// Signals sent out by MasterThreadProcess
extern const unsigned int FINISHMSG;/// Sent out when current jobs are finished
extern const unsigned int QUITMSG;/// Sent out when master_thread exits


struct ALLEGRO_THREAD;

void* MasterThreadProc(ALLEGRO_THREAD* thread , void* data);




class ThreadPool {
protected :
   
   std::unordered_set<Thread*> workers;
   std::deque<Thread*> jobs;
   std::map<THREADID , Thread*> complete_jobs;
   std::map<THREADID , Thread*> all_jobs;
   
   ALLEGRO_MUTEX* guard;
   
   ALLEGRO_THREAD* master_thread;/// Runs MasterThreadProc
   
   ALLEGRO_EVENT_QUEUE* queue;
   ALLEGRO_EVENT_QUEUE* fbqueue;
   
   ALLEGRO_EVENT_SOURCE* oureventsource;
   ALLEGRO_EVENT_SOURCE* usereventsource;
   
   int maxnthreads;
   bool running;
   bool paused;
   
   friend void* MasterThreadProc(ALLEGRO_THREAD* thread , void* data);
   
   unsigned int NWorkers();
   unsigned int NJobsWaiting();
   unsigned int MaxWorkers();

   Thread* TakeNextJob();

   void EnqueueJob(Thread* t);
   void FinishJob(Thread* t);

   void Guard();
   void Unguard();

public :
   ThreadPool();
   ~ThreadPool();

   void SetNumThreads(unsigned int N);
   
   THREADID AddJob(THREADPROC proc , void* data);
   
   void Start();
   void Pause();
   void Resume();
   
   void Finish();
   void Kill();
   
   void Abort();/// You really shoudln't call this. In fact I don't know why I coded it.
   
   void Dispose();

   unsigned int NumJobsLeft();

   Thread* GetThread(THREADID id);
   
   std::map<THREADID , Thread*> GetAllJobs();

   ALLEGRO_EVENT_SOURCE* EventSource() {return usereventsource;}
   ALLEGRO_EVENT_QUEUE* EventQueue() {return queue;}
};



#endif // ThreadPool_HPP
