#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_ask.H>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // Initialize FLTK
    Fl::scheme("gtk+"); // Use GTK+ theme for modern look
    
    // Create and show main window
    MainWindow *window = new MainWindow();
    window->show();
    
    // Run FLTK event loop
    return Fl::run();
}
