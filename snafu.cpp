#include <algorithm>
#include <string>
#include <iostream>
#include <iomanip>
#include <locale>
#include <map>
#include <csignal>
#include <ctime>
#include <vector>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

using std::wstring;

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

   wstring *getActiveWindowTitle() const;
};

wstring *TitleGrabber::getActiveWindowTitle() const
{
   Atom actual_type;
   int actual_format;
   unsigned long nitems;
   unsigned long bytes;
   long *data;
   int status;
   XTextProperty prop;
   wchar_t **title;
   int titles;
   wstring *result = 0;

   status = XGetWindowProperty(display, RootWindow(display, 0),
      XInternAtom(display, "_NET_ACTIVE_WINDOW", True), 0, (~0L), 
      False, AnyPropertyType, &actual_type, &actual_format, 
      &nitems, &bytes, reinterpret_cast<unsigned char **>(&data));
   if (status != Success)
      throw "error in XGetWindowProperty";
   if (data[0] == 0)
      goto err1;
   status = XGetWMName(display, data[0], &prop);
   XwcTextPropertyToTextList(display, &prop, &title, &titles);
   if (!title)
      goto err0;
   result = new wstring(title[0]);
   XFree(title[0]);
   XFree(title);
err0:
   XFree(prop.value);
err1:
   XFree(data);
   if (!result)
      result = new wstring(L"[bad window]");
   return result;
}

const unsigned int delay = 100000;
volatile sig_atomic_t goon = 1;
std::map<wstring, int> samples;
std::wstring previous;
int count = 0;
int changes = 0;
time_t tstart, tend;

bool compare(const std::pair<wstring, int> &p1, 
             const std::pair<wstring, int> &p2)
{
   return p1.second > p2.second;
}

void collect()
{
   time(&tstart);
   try {
      TitleGrabber grabber;
      while (goon) {
         wstring *title = grabber.getActiveWindowTitle();
         ++samples[*title];
         ++count;
         if (*title != previous) {
            ++changes;
            previous = *title;
         }
         delete title;
         usleep(delay);
      }
   } catch (const char *error) {
      std::cerr << error << std::endl;
   }
   time(&tend);
}

void showStatistics()
{
   std::wcout << "Start time: " << ctime(&tstart);
   std::wcout << "End time:   " << ctime(&tend);
   std::wcout << "# of ticks: " << count << std::endl;
   if (count == 0) {
      std::wcout << "No statistics were generated." << std::endl;
      return;
   }
   std::wcout << "Statistics of window frequency:" << std::endl;
   std::vector<std::pair<wstring, int> > vec;
   for (std::map<wstring, int>::const_iterator i = samples.begin(); 
        i != samples.end(); i++) 
   {
      vec.push_back(std::pair<wstring, int>(i->first, i->second));
   }
   sort(vec.begin(), vec.end(), compare);
   for (std::vector<std::pair<wstring, int> >::const_iterator i = vec.begin(); 
        i != vec.end(); i++) 
   {
      std::wcout << std::setw(6) 
                 << static_cast<double>(100 * i->second) / 
                    static_cast<double>(count) 
                 << "% --- " << i->first << std::endl;
   }
   std::wcout << std::endl;
   std::wcout << "Window changed every " 
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
   std::wcout.imbue(std::locale(""));
   std::ios::sync_with_stdio(false);
   std::wcout.setf(std::ios::fixed, std::ios::floatfield);
   std::wcout.setf(std::ios::internal, std::ios::adjustfield);
   std::wcout.precision(3);

   signal(SIGQUIT, handler);
   XSetErrorHandler(ignoreError);
   collect();
   showStatistics();
}
