



#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_color.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_ttf.h"

#include "Thread.hpp"
#include "Pool.hpp"


#include <string>


void* DoNothing(ALLEGRO_THREAD* t , void* d) {
   (void)t;
   al_rest(1.0);
   return d;
}


void Fail(char const *expr , char const *file , int line , char const *func) {
   int d = 1/0;
}

int main5(int argc , char** argv) {
   
   (void)argc;
   (void)argv;
   
   int nthreads = 100;
   
   if (argc > 1) {
      sscanf(argv[1] , "%d" , &nthreads);
   }
   
   if (!al_init()) {return -1;}
   
   al_register_assert_handler(Fail);
///   ALLEGRO_EVENT_QUEUE* q = al_create_event_queue();

   ALLEGRO_THREAD** threads = new ALLEGRO_THREAD*[nthreads];

   int bad = 0;
   for (int i = 0 ; i < nthreads ; ++i) {
      ALLEGRO_THREAD* t = threads[i] = al_create_thread(DoNothing , 0);
      if (!t) {
         bad++;
      }
      else {
         al_start_thread(t);
      }
   }
   printf("Added %d jobs, %d are bad.\n" , nthreads , bad);
   
   for (int i = 0 ; i < nthreads ; ++i) {
      if (threads[i]) {al_destroy_thread(threads[i]);}
   }
   
   delete threads;
   
   return 0;
}




class DATA {
public :
   ALLEGRO_BITMAP* bmp;
   std::string name;
   
   DATA() :
      bmp(0),
      name("")
   {}
};

void* SaveBitmap(ALLEGRO_THREAD* thread , void* data);


class Data {
public :
   int x;
   Data() : x(0) {static int i = 1;x = i++;}
};
void* Sum(ALLEGRO_THREAD* t , void* data) {
   return data;
   (void)t;
   Data* d = (Data*)data;
   int z = d->x;
   while (--z) {
      d->x += z;
   }
   return data;
}


int main(int argc , char** argv) {
   (void)argc;
   (void)argv;

   int NDAT = 300;

   if (argc > 1) {
      sscanf(argv[1] , "%d" , &NDAT);
   }
   
   if (!al_init()) {return -1;}

   al_register_assert_handler(Fail);

   Data* dat = new Data[NDAT];

   ThreadPool tpool;
   tpool.SetNumThreads(8);
   for (int i = 0 ; i < NDAT ; ++i) {
      tpool.AddJob(Sum , &dat[i]);
   }

   double tstart = al_get_time();
   tpool.Start();
   al_rest(1.0);
   tpool.Kill();
///   tpool.Finish();
   double tstop = al_get_time();
   double ttotal = tstop - tstart;


   for (int i = 0 ; i < NDAT ; ++i) {
      Data* d = &dat[i];
      printf("%d summed is %d.\n" , i + 1 , d->x);
   }

   printf("%d sums took %.6lf seconds.\n" , NDAT , ttotal);
   
   delete dat;
   
   return 0;
}

int main3(int argc , char** argv) {

   (void)argc;
   (void)argv;

   int jobs = 1;
   int bw = 64;
   int bh = 64;
   int nbitmaps = 50;
   
   bool usage = false;
   
   for (int a = 1 ; a < argc ; a++) {
      std::string arg = argv[a];
      if (arg.compare(0 , 2 , "-j") == 0) {
         if (1 != sscanf(arg.c_str() , "-j%d" , &jobs)) {
            printf("Couldn't read thread pool count.\n");
            usage = true;
         }
         if (jobs < 1) {jobs = 1;}
      }
      else if (arg.compare(0 , 2 , "-b") == 0) {
         if (2 != sscanf(arg.c_str() , "-b%dx%d" , &bw , &bh)) {
            printf("Couldn't read bitmap dimensions.\n");
            usage = true;
         }
         if (bw < 32) {bw = 32;}
         if (bh < 32) {bh = 32;}
      }
      else if (arg.compare(0 , 2 , "-n") == 0) {
         if (1 != sscanf(arg.c_str() , "-n%d" , &nbitmaps)) {
            printf("Couldn't read number of bitmaps.\n");
            usage = true;
         }
         if (nbitmaps < 10) {nbitmaps = 10;}
      }
      else if (arg.compare("-h") == 0) {
         usage = true;
      }
      else {
         printf("Unknown argument '%s'.\n" , arg.c_str());
         usage = true;
      }

   }
   
   if (usage) {
      printf("Usage for tpool :\n");
      printf("tpool [options]\n");
      printf("Options :\n");
      printf("-j# where # is the number of threads to use.\n");
      printf("-n# where # is the number of bitmaps to save.\n");
      printf("-b#x# where # is the width and height of the bitmap to save.\n");
      printf("-h shows help.\n");
      return -1;
   }
   
   printf("Saving %d bitmaps of size %d x %d using %d threads.\n" , nbitmaps , bw , bh , jobs);
   
   
   if (!al_init() || !al_init_image_addon() || !al_init_font_addon() || !al_init_ttf_addon()) {return 1;}
   
   if (!al_install_keyboard()) {return 2;}
   
   al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_OPENGL);
   ALLEGRO_DISPLAY* d = al_create_display(800,600);
   
   ALLEGRO_FONT* dfont = al_create_builtin_font();
   
   ALLEGRO_FONT* nicefont = al_load_ttf_font("Verdana.ttf" , -bh/2 , 0);
   
   ALLEGRO_FONT* font = nicefont?nicefont:dfont;
   
   DATA* dat = new DATA[nbitmaps];
   
   
   for (int i = 0 ; i < nbitmaps ; ++i) {
      al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
      ALLEGRO_BITMAP* bmp = al_create_bitmap(bw,bh);

      al_set_target_bitmap(bmp);
      float f = (float)i / nbitmaps;
      float h = 360.0f*f;
      float s = 1.0f;
      float l = f;
      al_clear_to_color(al_color_hsl(h,s,l));
      al_draw_textf(font , al_map_rgb(255,255,255) , bw/2 , bh/2 - al_get_font_line_height(font)/2 , ALLEGRO_ALIGN_CENTER , "%d" , i);

      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
      dat[i].bmp = al_clone_bitmap(bmp);
      al_destroy_bitmap(bmp);

      char buf[512] = {'\0'};
      sprintf(buf , "SavedImages/Bitmap%03d.png" , i);
      dat[i].name = buf;
///      al_save_bitmap(dat[i].name.c_str() , dat[i].bmp);
   }
///   return -5;
   
   al_set_target_backbuffer(d);
   al_clear_to_color(al_map_rgb(0,255,0));
   al_flip_display();
   
   printf("%d bitmaps created.\n" , nbitmaps);
   

   Thread thread;
   
   ThreadPool tpool;
//**
   
   tpool.SetNumThreads(jobs);
   
   for (int i = 0 ; i < nbitmaps ; ++i) {
      THREADID id = tpool.AddJob(SaveBitmap , &dat[i]);
      (void)id;
///      printf("Add Job returned %u.\n" , id);
   }

   ALLEGRO_EVENT_QUEUE* q = al_create_event_queue();
   
   al_register_event_source(q , tpool.EventSource());
   al_register_event_source(q , al_get_display_event_source(d));
   al_register_event_source(q , al_get_keyboard_event_source());

   if (0) {
      std::map<THREADID , Thread*> tmap = tpool.GetAllJobs();
      for (std::map<THREADID , Thread*>::iterator it = tmap.begin() ; it != tmap.end() ; ++it) {
         THREADID id = it->first;
         Thread* thread = it->second;
         printf("Thread ID %03u is at %p.\n" , id , thread);
      }
   }

   bool quit = false;
   bool finished = false;

   double tstart = al_get_time();
   double tstop = 0.0;
   double ttotal = 0.0;
   tpool.Start();
   
   while (!quit) {
      do {
         ALLEGRO_EVENT ev;
         al_wait_for_event(q , &ev);
         if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE || (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE)) {
            quit = true;
         }
         if (ev.type == FINISHMSG) {
            tstop = al_get_time();
            finished = true;
            quit = true;
            printf("ThreadPool finished in %.6lf seconds.\n" , tstop - tstart);
            break;
         }
         if (0) {
            if (ev.type == TSTARTMSG) {
               Thread* t = (Thread*)ev.user.data1;
               printf("Thread %03u at %p starting.\n" , t->ID() , t);
            }
            else if (ev.type == TSTOPMSG) {
               Thread* t = (Thread*)ev.user.data1;
               printf("Thread %03u at %p finished at %.6lf.\n" , t->ID() , t , t->StopTime());
            }
         }
      } while (!al_is_event_queue_empty(q));
   }
   ttotal = tstop - tstart;

   if (!finished) {
      tpool.Kill();
   }
   
   double total = 0.0;
   std::map<THREADID , Thread*> tmap = tpool.GetAllJobs();
   for (std::map<THREADID , Thread*>::iterator it = tmap.begin() ; it != tmap.end() ; ++it) {
///      THREADID id = it->first;
      Thread* thread = it->second;
///      printf("Thread ID %03u took %.6lf seconds.\n" , id , thread->RunTime());
      total += thread->RunTime();
   }
   double average = total / nbitmaps;
   printf("Average time of execution is %.6lf seconds. Total time is %.6lf seconds.\n" ,
           average , ttotal);
   
   tpool.Dispose();
   
   delete dat;

//*/
      
   return 0;
}



void* SaveBitmap(ALLEGRO_THREAD* thread , void* data) {
   (void)thread;
   DATA* dat = (DATA*)data;
   al_save_bitmap(dat->name.c_str() , dat->bmp);
///   ALLEGRO_BITMAP* bmp = dat->bmp;
///   ALLEGRO_LOCKED_REGION* lock = al_lock_bitmap(bmp , ALLEGRO_PIXEL_FORMAT_ARGB_8888 , ALLEGRO_LOCK_READONLY);
///   printf("%s lock on bitmap %p.\n" , lock?"Acquired":"Failed to acquire" , bmp);
///   if (lock) {al_unlock_bitmap(bmp);}
   return data;
}



