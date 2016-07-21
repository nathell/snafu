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
#include <vector>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define STRING_FILE "strings.dat"
#define DATA_FILE "windows.dat"

using std::string;
using std::map;
using std::vector;
using std::FILE;
using std::fopen;

class TitleGrabber
{
private:
   Display *display;

public:
   TitleGrabber(const char *name = 0) {
      display = XOpenDisplay(name);
      if (!display)
         throw "Can't open display";
   }

   ~TitleGrabber() {
      XCloseDisplay(display);
   }

   string *getActiveWindowTitle() const;
};

string *TitleGrabber::getActiveWindowTitle() const
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
   try {
      TitleGrabber grabber;
      time_t tm;
      Entry entry;
      while (goon) {
         string *title = grabber.getActiveWindowTitle();
         ++samples[*title];
         ++count;
         if (*title != previous) {
            ++changes;
            previous = *title;
         }
         entry.str = set.intern(title);
         time(&tm);
         entry.time = static_cast<int32_t>(tm);
         fwrite(&entry, sizeof(entry), 1, f);
         fflush(f);
         set.save(STRING_FILE);
         usleep(delay);
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

   signal(SIGQUIT, handler);
   XSetErrorHandler(ignoreError);
   collect();
   showStatistics();
}

// Local variables:
// c-basic-offset: 3
// End:
