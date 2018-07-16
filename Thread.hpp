



#ifndef Thread_HPP
#define Thread_HPP


/// Signals sent by BypassProcess when the thread is started or stopped

extern const unsigned int TSTARTMSG;
extern const unsigned int TSTOPMSG;


/// Simple id to track threads

typedef unsigned int THREADID;

extern const THREADID BADTHREADID;




struct ALLEGRO_THREAD;
struct ALLEGRO_EVENT_SOURCE;

typedef void* (*THREADPROC)(ALLEGRO_THREAD* , void*);

void* BypassProcess(ALLEGRO_THREAD* athread , void* data);



class Thread {
protected :
   ALLEGRO_EVENT_SOURCE* evsrc;
   ALLEGRO_THREAD* athread;
   THREADPROC tproc;
   void* tdata;
   void* rdata;
   THREADID tid;
   bool running;
   double tstart;
   double tstop;
   
   
   friend void* BypassProcess(ALLEGRO_THREAD* athread , void* data);
   
   void Free();

public :
   Thread();
   ~Thread();
   bool Create(THREADPROC proc , void* data);
   void Run();
   void Finish();
   void Kill();

   void* Data() {return tdata;}
   ALLEGRO_EVENT_SOURCE* EventSource() {return evsrc;}
   THREADID ID() {return tid;}
   double RunTime();
   double StopTime();
   double StartTime();
};




#endif // Thread_HPP

