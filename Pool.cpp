


#include "Pool.hpp"


#include "allegro5/allegro.h"





const unsigned int STOPMSG = ALLEGRO_GET_EVENT_TYPE('S' , 'T' , 'O' , 'P');
const unsigned int STARTMSG = ALLEGRO_GET_EVENT_TYPE('S' , 'T' , 'R' , 'T');
const unsigned int FINISHMSG = ALLEGRO_GET_EVENT_TYPE('F' , 'N' , 'S' , 'H');
const unsigned int CONTINUEMSG = ALLEGRO_GET_EVENT_TYPE('C' , 'O' , 'N' , 'T');
const unsigned int KILLMSG = ALLEGRO_GET_EVENT_TYPE('K' , 'I' , 'L' , 'L');
const unsigned int QUITMSG = ALLEGRO_GET_EVENT_TYPE('Q' , 'U' , 'I' , 'T');




unsigned int ThreadPool::NWorkers() {
   return workers.size();
}



unsigned int ThreadPool::NJobsWaiting() {
   return jobs.size();
}



unsigned int ThreadPool::MaxWorkers() {
   return maxnthreads;
}



Thread* ThreadPool::TakeNextJob() {
   Thread* t = 0;
   if (jobs.size()) {
      t = jobs.front();
      jobs.pop_front();
   }
   return t;
}



void ThreadPool::EnqueueJob(Thread* t) {
///   printf("Inserting Thread %p into workers.\n" , t);
   workers.insert(t);
   t->Start();
}



void ThreadPool::FinishJob(Thread* t) {
   Guard();
   complete_jobs[t->ID()] = t;
///   printf("Erasing Thread %p from workers.\n" , t);
   workers.erase(workers.find(t));
   Unguard();

///   delete t;/// t must remain valid for the user to extract data from it
}



void ThreadPool::Guard() {
   al_lock_mutex(guard);
}



void ThreadPool::Unguard() {
   al_unlock_mutex(guard);
}



void ThreadPool::DisposeAll() {
   for (std::map<THREADID , Thread*>::iterator it = all_jobs.begin() ; it != all_jobs.end() ; ++it) {
      delete it->second;
   }
   all_jobs.clear();
   complete_jobs.clear();
}



ThreadPool::ThreadPool() :
      workers(),
      jobs(),
      guard(0),
      master_thread(0),
      queue(0),
      fbqueue(0),
      oureventsource(0),
      usereventsource(0),
      maxnthreads(1),
      running(false),
      paused(true)
{

   oureventsource = new ALLEGRO_EVENT_SOURCE();
   al_init_user_event_source(oureventsource);

   usereventsource = new ALLEGRO_EVENT_SOURCE();
   al_init_user_event_source(usereventsource);

   queue = al_create_event_queue();
   al_register_event_source(queue , oureventsource);

   fbqueue = al_create_event_queue();
   al_register_event_source(fbqueue , oureventsource);

   guard = al_create_mutex();

   master_thread = al_create_thread(MasterThreadProc , this);

   al_start_thread(master_thread);
}



ThreadPool::~ThreadPool() {
   if (running) {
      Kill();
   }

   DisposeAll();

   al_destroy_thread(master_thread);
   master_thread = 0;

   al_destroy_mutex(guard);
   guard = 0;

   al_destroy_event_queue(queue);
   queue = 0;

   al_destroy_event_queue(fbqueue);
   fbqueue = 0;

   al_destroy_user_event_source(oureventsource);
   delete oureventsource;
   oureventsource = 0;

   al_destroy_user_event_source(usereventsource);
   delete usereventsource;
   usereventsource = 0;
}



void ThreadPool::SetNumThreads(unsigned int N) {
   maxnthreads = N?N:1;
}



THREADID ThreadPool::AddJob(THREADPROC proc , void* data) {
   Thread* t = new Thread();
   t->Setup(proc , data);
   al_register_event_source(queue , t->EventSource());

   Guard();
   jobs.push_back(t);
   all_jobs[t->ID()] = t;
   Unguard();

   /// In case our queue is empty and we're waiting on an event, send a dummy event
   if (!paused) {
      ALLEGRO_EVENT ev;
      ev.type = CONTINUEMSG;
      ev.user.data1 = (intptr_t)this;
      al_emit_user_event(oureventsource , &ev , 0);
   }

   return t->ID();
}



void ThreadPool::Start() {
   Resume();
}



void ThreadPool::Pause() {
   ALLEGRO_EVENT ev;
   ev.type = STOPMSG;
   ev.user.data1 = (intptr_t)this;
   al_emit_user_event(oureventsource , &ev , 0);
}



void ThreadPool::Resume() {
   ALLEGRO_EVENT ev;
   ev.type = STARTMSG;
   ev.user.data1 = (intptr_t)this;
   al_emit_user_event(oureventsource , &ev , 0);
}



void ThreadPool::Finish() {
   ALLEGRO_EVENT rev;
   do {
      al_wait_for_event(fbqueue , &rev);
   } while (rev.type != FINISHMSG);
}



void ThreadPool::Kill() {
   ALLEGRO_EVENT ev;
   ev.type = KILLMSG;
   ev.user.data1 = (intptr_t)this;
   al_emit_user_event(oureventsource , &ev , 0);

   ALLEGRO_EVENT rev;
   do {
      al_wait_for_event(fbqueue , &rev);
   } while (rev.type != QUITMSG);
}



void ThreadPool::Abort() {
   al_set_thread_should_stop(master_thread);
   Kill();
}



void ThreadPool::Dispose() {
   Guard();
   for (std::map<THREADID , Thread*>::iterator it = complete_jobs.begin() ; it != complete_jobs.end() ; ++it) {
      Thread* thread = it->second;
      all_jobs.erase(all_jobs.find(thread->ID()));
      delete thread;
   }
   complete_jobs.clear();
   Unguard();
}



unsigned int ThreadPool::NumJobsLeft() {
   int njobs = -1;
   Guard();
   njobs = NWorkers() + NJobsWaiting();
   Unguard();
   return njobs;
}



Thread* ThreadPool::GetThread(THREADID id) {
   Thread* t = 0;
   Guard();
   std::map<THREADID , Thread*>::iterator it = all_jobs.find(id);
   if (it != all_jobs.end()) {
      t = it->second;
   }
   Unguard();
   return t;
}



std::map<THREADID , Thread*> ThreadPool::GetAllJobs() {
   std::map<THREADID , Thread*> jobs;
   Guard();
   jobs = all_jobs;
   Unguard();
   return jobs;
}



void* MasterThreadProc(ALLEGRO_THREAD* thread , void* data) {

   ThreadPool* tpool = (ThreadPool*)data;
   if (!tpool) {return 0;}

   ALLEGRO_EVENT_QUEUE* q = tpool->EventQueue();

   bool kill = false;

   tpool->running = true;

   while (!al_get_thread_should_stop(thread) && !kill) {

      /// Start new threads, up to limit
      if (!tpool->paused) {
         Thread* next = 0;
         tpool->Guard();
///         printf("Queueing workers.\n");
         while ((tpool->NWorkers() < tpool->MaxWorkers()) && (next = tpool->TakeNextJob())) {
            tpool->EnqueueJob(next);
         }
         tpool->Unguard();
      }


      /// Wait for an event, from a thread or from main or wherever
      ALLEGRO_EVENT ev;
      do {
         al_wait_for_event(q , &ev);

         /// A thread notified us that it is starting
         if (ev.type == TSTARTMSG) {
            Thread* t = (Thread*)ev.user.data1;
            (void)t;
            printf("MTPROC received TSTARTMSG from THREADID %03u at %p.\n" , t->ID() , t);

            /// Relay event to user
            al_emit_user_event(tpool->usereventsource , &ev , 0);
         }

         /// A thread notified us that it has stopped
         if (ev.type == TSTOPMSG) {
            Thread* t = (Thread*)ev.user.data1;
            printf("MTPROC received TSTOPMSG from THREADID %03u at %p.\n" , t->ID() , t);
            assert(t == tpool->GetThread(t->ID()));

            tpool->FinishJob(t);

            /// Relay event to user
            al_emit_user_event(tpool->usereventsource , &ev , 0);

            /// If our job queue is now empty, emit an event
            int njobs = tpool->NumJobsLeft();
            printf("NJOBS left = %d\n" , njobs);
            if (njobs == 0) {
               ALLEGRO_EVENT ev2;
               ev2.type = FINISHMSG;
               ev2.user.data1 = (intptr_t)tpool;
               al_emit_user_event(tpool->oureventsource , &ev2 , 0);
               al_emit_user_event(tpool->usereventsource , &ev2 , 0);
            }
         }
         if (ev.type == STARTMSG) {
///            printf("MTPROC received STARTMSG.\n");
            tpool->paused = false;
         }
         if (ev.type == STOPMSG) {
///            printf("MTPROC received STOPMSG.\n");
            tpool->paused = true;
         }
         if (ev.type == KILLMSG) {
///            printf("MTPROC received KILLMSG.\n");
            tpool->paused = true;
            kill = true;
            std::unordered_set<Thread*> workers;
            tpool->Guard();
            workers = tpool->workers;
            tpool->Unguard();
            for (std::unordered_set<Thread*>::iterator it = workers.begin() ; it != workers.end() ; ++it) {
               Thread* t = *it;
               t->Kill();
            }
         }
         if (ev.type == CONTINUEMSG) {
///            printf("MTPROC received CONTINUEMSG.\n");
            break;/// out of the message loop so we can process the job queue
         }
      } while (!al_is_event_queue_empty(q) && !kill);/// Process all current events

   }///   while (!al_thread_should_stop(thread) && !kill) {

   tpool->running = false;

   ALLEGRO_EVENT ev;
   ev.type = QUITMSG;
   ev.user.data1 = (intptr_t)tpool;
   al_emit_user_event(tpool->oureventsource , &ev , 0);

   return data;
}




