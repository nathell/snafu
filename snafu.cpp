#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <locale>
#include <map>
#include <csignal>
#include <ctime>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>

#define STRING_FILE "strings.dat"
#define DATA_FILE "windows.dat"

using std::string;
using std::map;
using std::vector;
using std::FILE;
using std::fopen;

Display *display;

string *getActiveWindowTitle()
{
   Atom actual_type;
   int actual_format;
   unsigned long nitems;
   unsigned long bytes;
   long *data;
   int status;
   XTextProperty prop;
   char **title;
   int titles;
   string *result = 0;

   status = XGetWindowProperty(display, RootWindow(display, 0),
      XInternAtom(display, "_NET_ACTIVE_WINDOW", True), 0, (~0L),
      False, AnyPropertyType, &actual_type, &actual_format,
      &nitems, &bytes, reinterpret_cast<unsigned char **>(&data));
   if (status != Success)
      throw "error in XGetWindowProperty";
   if (data[0] == 0)
      goto err1;
   status = XGetWMName(display, data[0], &prop);
   Xutf8TextPropertyToTextList(display, &prop, &title, &titles);
   if (!title)
      goto err0;
   result = new string(title[0]);
   XFree(title[0]);
   XFree(title);
err0:
   XFree(prop.value);
err1:
   XFree(data);
   if (!result)
      result = new string("[bad window]");
   return result;
}

void select_events()
{
   XIEventMask evmasks[1];
   unsigned char mask1[(XI_LASTEVENT + 7)/8];

   int xi_opcode, event, error;

   if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &event, &error)) {
      throw "xi2 not available";
   }

   memset(mask1, 0, sizeof(mask1));

   /* select for button and key events from all master devices */
   XISetMask(mask1, XI_ButtonPress);
   XISetMask(mask1, XI_ButtonRelease);
   XISetMask(mask1, XI_KeyPress);
   XISetMask(mask1, XI_KeyRelease);
   XISetMask(mask1, XI_Motion);

   evmasks[0].deviceid = XIAllDevices;
   evmasks[0].mask_len = sizeof(mask1);
   evmasks[0].mask = mask1;

   XISelectEvents(display, DefaultRootWindow(display), evmasks, 1);
   XSync(display, False);
   XFlush(display);
}

class StringSet {
   struct comparator {
      bool operator()(const string* s1, const string* s2) const {
         return *s1 < *s2;
      }
   };

   typedef map<string*, int, comparator> mymap;

   mymap mapping;
   vector<string*> holder;

public:

   ~StringSet() {
      for (vector<string*>::iterator it = holder.begin(); it != holder.end(); it++) {
         delete *it;
      }
   }

   int intern(string* str) {
      mymap::iterator it = mapping.find(str);
      int res = -1;
      if (it == mapping.end()) {
         res = mapping.size();
         holder.push_back(str);
         mapping[str] = res;
      } else {
         delete str;
         res = it->second;
      }
      return res;
   }

   int intern(const char* str) {
      string* wstr = new string(str);
      return intern(wstr);
   }

   void save(const char *outfile) {
      uint32_t currentsize = 0, sz = 0;

      FILE *f;
      f = fopen(outfile, "r+b");
      if (f == NULL) {
         f = fopen(outfile, "wb");
      }

      if (fread(&sz, sizeof(uint32_t), 1, f) == 1)
         currentsize = sz;

      if (currentsize < holder.size()) {
         sz = holder.size();
         fseek(f, 0L, SEEK_SET);
         fwrite(&sz, sizeof(uint32_t), 1, f);
         fseek(f, 0L, SEEK_END);

         for (size_t i = currentsize; i < holder.size(); i++) {
            fprintf(f, "%s", holder[i]->c_str());
            fputc(0, f);
         }
      }

      fclose(f);
   }

   void load(const char *infile) {
      uint32_t sz;
      char *s = NULL;
      size_t n = 0;

      FILE *f = fopen(infile, "rb");
      if (f == NULL)
         return;

      if (fread(&sz, sizeof(uint32_t), 1, f) == 0) return;
      for (size_t i = 0; i < sz; i++) {
         if (getdelim(&s, &n, 0, f) < 0) return;
         intern(new string(s));
      }
      free(s);
   }
};

const unsigned int delay = 1000000;
volatile sig_atomic_t goon = 1;
std::map<string, int> samples;
std::string previous;
int count = 0;
int changes = 0;
time_t tstart, tend;

bool compare(const std::pair<string, int> &p1,
             const std::pair<string, int> &p2)
{
   return p1.second > p2.second;
}

struct Entry {
   int32_t time;
   uint32_t str;
};

void collect()
{
   time(&tstart);
   StringSet set;
   set.load(STRING_FILE);
   FILE *f = fopen(DATA_FILE, "a+b");
   select_events();
   try {
      time_t tm;
      Entry entry;
      while (goon) {
         XEvent ev;
         int have_event = 0;
         while (XPending(display)) {
            have_event = 1;
            XNextEvent(display, &ev);
         }
         string *title = getActiveWindowTitle();
         ++samples[*title];
         ++count;
         if (*title != previous) {
            ++changes;
            previous = *title;
         }
         entry.str = set.intern(title);
         time(&tm);
         entry.time = static_cast<int32_t>(tm) | (have_event << 31);
         fwrite(&entry, sizeof(entry), 1, f);
         fflush(f);
         set.save(STRING_FILE);
         usleep(1000000);
      }
   } catch (const char *error) {
      std::cerr << error << std::endl;
   }
   fclose(f);
   time(&tend);
}

void showStatistics()
{
   std::cout << "Start time: " << ctime(&tstart);
   std::cout << "End time:   " << ctime(&tend);
   std::cout << "# of ticks: " << count << std::endl;
   if (count == 0) {
      std::cout << "No statistics were generated." << std::endl;
      return;
   }
   std::cout << "Statistics of window frequency:" << std::endl;
   std::vector<std::pair<string, int> > vec;
   for (std::map<string, int>::const_iterator i = samples.begin();
        i != samples.end(); i++)
   {
      vec.push_back(std::pair<string, int>(i->first, i->second));
   }
   sort(vec.begin(), vec.end(), compare);
   for (std::vector<std::pair<string, int> >::const_iterator i = vec.begin();
        i != vec.end(); i++)
   {
      std::cout << std::setw(6)
                 << static_cast<double>(100 * i->second) /
                    static_cast<double>(count)
                 << "% --- " << i->first << std::endl;
   }
   std::cout << std::endl;
   std::cout << "Window changed every "
              << static_cast<double>(count * delay) / (1000000.0 * changes)
              << " seconds" << std::endl;
}

void handler(int sig)
{
   goon = 0;
}

int ignoreError(Display *display, XErrorEvent *error)
{
   std::cerr << "punt!" << std::endl;
   return 0;
}

int main()
{
   std::locale::global(std::locale(""));
   std::cout.imbue(std::locale(""));
   std::ios::sync_with_stdio(false);
   std::cout.setf(std::ios::fixed, std::ios::floatfield);
   std::cout.setf(std::ios::internal, std::ios::adjustfield);
   std::cout.precision(3);

   display = XOpenDisplay(0);
   if (!display)
      throw "Can't open display";

   signal(SIGQUIT, handler);
   signal(SIGINT, handler);
   XSetErrorHandler(ignoreError);
   collect();
   XCloseDisplay(display);
   showStatistics();
}

// Local variables:
// c-basic-offset: 3
// End:
