#include "MainWindow.h"
#include <gtkmm/application.h>

int main(int argc, char *argv[]) {
    auto app = Gtk::Application::create(argc, argv, "com.maskedsyntax.stick");
    
    // Set default icon name (looks for 'stick.png' in system icon paths)
    Gtk::Window::set_default_icon_name("stick");

    MainWindow window;
    return app->run(window);
}
