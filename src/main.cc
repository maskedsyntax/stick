#include "MainWindow.h"
#include <gtkmm/application.h>

int main(int argc, char *argv[]) {
    auto app = Gtk::Application::create(argc, argv, "com.example.stick");
    MainWindow window;
    return app->run(window);
}