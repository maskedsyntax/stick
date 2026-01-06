#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gtkmm.h>
#include <string>
#include <vector>

class MainWindow : public Gtk::Window {
public:
    MainWindow();
    virtual ~MainWindow();

protected:
    void on_iso_file_set();
    void on_drive_changed();
    void on_flash_clicked();
    void on_refresh_clicked();

    void refresh_drives();
    void start_flashing();
    bool on_flash_output(Glib::IOCondition condition);
    void on_flash_finished(int exit_code);
    void parse_progress(const std::string& line);
    void show_error(const std::string& message);
    void show_info(const std::string& message);
    void set_ui_sensitive(bool sensitive);

    Gtk::Box m_vbox;

    Gtk::Box m_iso_box;
    Gtk::Label m_iso_label;
    Gtk::FileChooserButton m_iso_chooser;

    Gtk::Box m_drive_box;
    Gtk::Label m_drive_label;
    Gtk::ComboBoxText m_drive_combo;
    Gtk::Button m_refresh_button;

    Gtk::Label m_status_label;
    Gtk::ProgressBar m_progress_bar;
    Gtk::Button m_flash_button;

    // State
    bool m_flashing_done; // Track if we are in "Eject" mode
    struct DriveInfo {
        std::string device_path; // /dev/sdb
        std::string name;        // sdb
        std::string model;       // SanDisk...
        std::string size;        // 16G
        std::string label;       // display text
    };
    std::vector<DriveInfo> m_drives;

    pid_t m_pid;
    int m_fd_out;
    int m_fd_err; // dd status=progress goes to stderr
    sigc::connection m_io_connection;
    long m_iso_size; // in bytes
};

#endif // MAINWINDOW_H
