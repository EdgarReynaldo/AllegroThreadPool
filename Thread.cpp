



#include "Thread.hpp"

#include "allegro5/allegro.h"


#include <cstdio>



const unsigned int TSTOPMSG = ALLEGRO_GET_EVENT_TYPE('T' , 'S' , 'T' , 'O');
const unsigned int TSTARTMSG = ALLEGRO_GET_EVENT_TYPE('T' , 'S' , 'T' , 'A');




const THREADID BADTHREADID = ~0;



static THREADID NextThreadID() {
   static THREADID id = 0;
   return id++;
}



void* BypassProcess(ALLEGRO_THREAD* athread , void* data) {
   Thread* thread = (Thread*)data;
   
   if (thread->tid == 860) {
      printf("Thread 860 starting.\n");
   }

   ALLEGRO_EVENT ev;
   ev.type = TSTARTMSG;
   ev.user.data1 = (intptr_t)thread;
   al_emit_user_event(thread->evsrc , &ev , 0);

   thread->tstart = al_get_time();
   thread->running = true;
   thread->rdata = thread->tproc(athread , thread->tdata);
   thread->running = false;
   thread->tstop = al_get_time();
   
   ALLEGRO_EVENT ev2;
   ev2.type = TSTOPMSG;
   ev2.user.data1 = (intptr_t)thread;
   al_emit_user_event(thread->evsrc , &ev2 , 0);
   
   if (thread->tid == 860) {
      printf("Thread 860 exiting.\n");
   }
   
   return thread->rdata;
}



void Thread::Free() {
   if (athread) {Finish();}
   al_destroy_thread(athread);
   athread = 0;
}



Thread::Thread() :
      evsrc(0),
      athread(0),
      tproc(0),
      tdata(0),
      rdata(0),
      tid(NextThreadID()),
      running(false),
      tstart(0.0),
      tstop(0.0)
{
   evsrc = new ALLEGRO_EVENT_SOURCE();
   al_init_user_event_source(evsrc);
}



Thread::~Thread() {
   Free();
   al_destroy_user_event_source(evsrc);
   delete evsrc;
   evsrc = 0;
}



bool Thread::Create(THREADPROC proc , void* data) {
   Free();
   tproc = proc;
   tdata = data;
   athread = al_create_thread(BypassProcess , this);
   return athread;
}



void Thread::Run() {
   al_start_thread(athread);
}



void Thread::Finish() {
   al_join_thread(athread , &tdata);
}



void Thread::Kill() {
   al_set_thread_should_stop(athread);
   Finish();
}



double Thread::RunTime() {
   return StopTime() - StartTime();
}



double Thread::StopTime() {
   return tstop;
}



double Thread::StartTime() {
   return tstart;
}
