#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // Initialize FLTK
    Fl::scheme("plastic"); // Use a modern-looking scheme
    
    // Create and show main window
    MainWindow *window = new MainWindow();
    window->size_range(800, 600, 0, 0);
    window->show();
    
    // Run FLTK event loop
    return Fl::run();
}
