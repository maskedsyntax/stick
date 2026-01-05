#include "MainWindow.h"
#include <iostream>
#include <regex>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <fstream>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>

// Helper to execute shell command and get output (for scanning)
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

MainWindow::MainWindow() 
    : m_vbox(Gtk::ORIENTATION_VERTICAL, 12),
      m_iso_box(Gtk::ORIENTATION_HORIZONTAL, 10),
      m_drive_box(Gtk::ORIENTATION_HORIZONTAL, 10),
      m_flashing_done(false),
      m_pid(-1),
      m_fd_out(-1),
      m_fd_err(-1),
      m_iso_size(0)
{
    set_title("Stick");
    set_default_size(450, 220);
    set_border_width(15);

    // --- UI Setup ---

    // ISO Section
    m_iso_label.set_text("ISO Image:");
    m_iso_label.set_halign(Gtk::ALIGN_START);
    
    m_iso_chooser.set_title("Select ISO File");
    auto filter = Gtk::FileFilter::create();
    filter->set_name("ISO Images");
    filter->add_pattern("*.iso");
    filter->add_pattern("*.img");
    m_iso_chooser.add_filter(filter);
    m_iso_chooser.set_hexpand(true);
    m_iso_chooser.signal_file_set().connect(sigc::mem_fun(*this, &MainWindow::on_iso_file_set));

    m_iso_box.pack_start(m_iso_label, Gtk::PACK_SHRINK);
    m_iso_box.pack_start(m_iso_chooser, Gtk::PACK_EXPAND_WIDGET);

    // Drive Section
    m_drive_label.set_text("Target USB:");
    m_drive_label.set_halign(Gtk::ALIGN_START);

    m_drive_combo.set_hexpand(true);
    m_drive_combo.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_drive_changed));

    m_refresh_button.set_image_from_icon_name("view-refresh-symbolic", Gtk::ICON_SIZE_BUTTON);
    m_refresh_button.set_tooltip_text("Refresh Drives");
    m_refresh_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::refresh_drives));

    m_drive_box.pack_start(m_drive_label, Gtk::PACK_SHRINK);
    m_drive_box.pack_start(m_drive_combo, Gtk::PACK_EXPAND_WIDGET);
    m_drive_box.pack_start(m_refresh_button, Gtk::PACK_SHRINK);

    // Status Label
    m_status_label.set_markup("<b>Ready to flash</b>");
    m_status_label.set_halign(Gtk::ALIGN_CENTER);
    m_status_label.set_margin_top(5);

    // Progress Bar
    m_progress_bar.set_show_text(false);
    m_progress_bar.set_fraction(0.0);

    // Flash Button
    m_flash_button.set_label("Flash!");
    m_flash_button.set_sensitive(false);
    m_flash_button.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_flash_clicked));

    // Layout
    m_vbox.pack_start(m_iso_box, Gtk::PACK_SHRINK);
    m_vbox.pack_start(m_drive_box, Gtk::PACK_SHRINK);
    m_vbox.pack_start(m_status_label, Gtk::PACK_SHRINK);
    m_vbox.pack_start(m_progress_bar, Gtk::PACK_SHRINK);
    m_vbox.pack_end(m_flash_button, Gtk::PACK_SHRINK);

    add(m_vbox);

    refresh_drives();
    show_all_children();
}
MainWindow::~MainWindow() {
    if (m_pid > 0) {
        // Cleanup if closed while running
        kill(m_pid, SIGTERM);
        int status;
        waitpid(m_pid, &status, WNOHANG);
    }
}

void MainWindow::refresh_drives() {
    m_drive_combo.remove_all();
    m_drives.clear();

    try {
        std::string output = exec("lsblk -P -d -o NAME,SIZE,MODEL,RM,TYPE,TRAN");
        std::istringstream stream(output);
        std::string line;
        
        std::regex re("NAME=\"([^\"]*)\"\\s+SIZE=\"([^\"]*)\"\\s+MODEL=\"([^\"]*)\"\\s+RM=\"([^\"]*)\"\\s+TYPE=\"([^\"]*)\"\\s+TRAN=\"([^\"]*)\"");
        
        while (std::getline(stream, line)) {
            std::smatch match;
            if (std::regex_search(line, match, re)) {
                std::string name = match[1];
                std::string size = match[2];
                std::string model = match[3];
                std::string rm = match[4];
                std::string type = match[5];
                std::string tran = match[6];

                bool is_removable = (rm == "1");
                bool is_usb = (tran == "usb");

                if ((is_removable || is_usb) && type == "disk") {
                    DriveInfo info;
                    info.device_path = "/dev/" + name;
                    info.name = name;
                    info.model = model;
                    info.size = size;
                    info.label = info.device_path + " - " + model + " (" + size + ")";
                    
                    m_drives.push_back(info);
                    m_drive_combo.append(info.label);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning drives: " << e.what() << std::endl;
    }

    if (m_drives.empty()) {
        m_drive_combo.append("No removable drives found");
        m_drive_combo.set_active(0);
        m_drive_combo.set_sensitive(false);
    } else {
        m_drive_combo.set_active(0);
        m_drive_combo.set_sensitive(true);
    }
    
    on_drive_changed();
}

void MainWindow::on_iso_file_set() {
    std::string filename = m_iso_chooser.get_filename();
    if (!filename.empty()) {
        try {
            m_iso_size = std::filesystem::file_size(filename);
            m_status_label.set_text("Ready to flash " + std::filesystem::path(filename).filename().string());
        } catch (...) {}
    }
    on_drive_changed();
}

void MainWindow::on_drive_changed() {
    if (m_flashing_done) return; // Don't reset if we are in done state

    bool iso_selected = !m_iso_chooser.get_filename().empty();
    bool drive_selected = !m_drives.empty() && m_drive_combo.get_active_row_number() >= 0;
    
    m_flash_button.set_sensitive(iso_selected && drive_selected);
}

void MainWindow::start_flashing() {
    int drive_idx = m_drive_combo.get_active_row_number();
    if (drive_idx < 0 || drive_idx >= (int)m_drives.size()) return;

    std::string iso_path = m_iso_chooser.get_filename();
    std::string device_path = m_drives[drive_idx].device_path;

    if (iso_path.empty() || device_path.empty()) return;

    if (device_path == "/dev/sda") {
        Gtk::MessageDialog dialog(*this, "Warning: Target is /dev/sda", false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_YES_NO);
        dialog.set_secondary_text("This is often the system drive. Are you absolutely sure?");
        if (dialog.run() != Gtk::RESPONSE_YES) return;
    }

    Gtk::MessageDialog dialog(*this, "Confirm Flash", false, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL);
    dialog.set_secondary_text("This will erase ALL data on " + device_path + "\nAre you sure you want to continue?");
    if (dialog.run() != Gtk::RESPONSE_OK) return;

    std::vector<std::string> argv;
    argv.push_back("pkexec");
    argv.push_back("bash");
    argv.push_back("-c");
    // Use conv=fsync for data integrity at the end, status=progress for parsing
    std::string cmd = "dd if=\"" + iso_path + "\" of=\"" + device_path + "\" bs=4M status=progress conv=fsync";
    argv.push_back(cmd);

    try {
        Glib::spawn_async_with_pipes(
            "", 
            argv, 
            Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH,
            Glib::SlotSpawnChildSetup(),
            &m_pid,
            nullptr, 
            nullptr, 
            &m_fd_err 
        );

        set_ui_sensitive(false);
        m_status_label.set_text("Starting...");
        m_progress_bar.set_fraction(0.0);
        
        m_flash_button.set_label("Flashing...");

        m_io_connection = Glib::signal_io().connect(
            sigc::mem_fun(*this, &MainWindow::on_flash_output),
            m_fd_err,
            Glib::IO_IN | Glib::IO_HUP
        );

    } catch (const Glib::Error& ex) {
        show_error("Failed to start flashing: " + ex.what());
    }
}

void MainWindow::on_flash_clicked() {
    if (m_flashing_done) {
        // Eject Mode
        int drive_idx = m_drive_combo.get_active_row_number();
        if (drive_idx >= 0 && drive_idx < (int)m_drives.size()) {
            std::string device_path = m_drives[drive_idx].device_path;
            std::string cmd = "eject " + device_path;
            if (system(cmd.c_str()) != 0) {
                // Ignore error, maybe already ejected
            }
            show_info("Drive ejected. You can now remove it.");
            
            // Reset state
            m_flashing_done = false;
            m_flash_button.set_label("Flash!");
            
            m_status_label.set_text("Ready to flash");
            m_progress_bar.set_fraction(0.0);
            set_ui_sensitive(true);
        }
        return;
    }

    start_flashing();
}

bool MainWindow::on_flash_output(Glib::IOCondition condition) {
    if (condition & Glib::IO_IN) {
        char buffer[1024];
        ssize_t bytes_read = ::read(m_fd_err, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string line(buffer);
            parse_progress(line);
        }
    }

    if (condition & Glib::IO_HUP) {
        int status = 0;
        waitpid(m_pid, &status, 0);
        on_flash_finished(WEXITSTATUS(status));
        return false; 
    }

    return true; 
}

void MainWindow::parse_progress(const std::string& data) {
    static std::regex re(R"((\d+) bytes)");
    std::smatch match;
    if (std::regex_search(data, match, re)) {
        try {
            long bytes_written = std::stol(match[1]);
            if (m_iso_size > 0) {
                double fraction = (double)bytes_written / (double)m_iso_size;
                if (fraction > 1.0) fraction = 1.0;
                m_progress_bar.set_fraction(fraction);
                
                int percent = (int)(fraction * 100);
                if (percent >= 100) {
                     m_status_label.set_markup("<b>Syncing data... DO NOT REMOVE!</b>");
                } else {
                     m_status_label.set_text("Flashing: " + std::to_string(percent) + "%");
                }
            }
        } catch (...) {}
    }
}

void MainWindow::on_flash_finished(int exit_code) {
    m_pid = -1;
    m_io_connection.disconnect();
    if (m_fd_err != -1) {
        ::close(m_fd_err);
        m_fd_err = -1;
    }

    if (exit_code == 0) {
        m_progress_bar.set_fraction(1.0);
        m_status_label.set_text("Flashing Completed!");
        
        // Enter Eject Mode
        m_flashing_done = true;
        m_flash_button.set_sensitive(true);
        m_flash_button.set_label("Eject Drive");
        
    } else {
        set_ui_sensitive(true);
        m_status_label.set_text("Flashing Failed");
        if (exit_code == 126 || exit_code == 127) {
             show_error("Authentication failed or cancelled.");
        } else {
             show_error("Flashing failed with exit code: " + std::to_string(exit_code));
        }
    }
}

void MainWindow::set_ui_sensitive(bool sensitive) {
    m_iso_chooser.set_sensitive(sensitive);
    m_drive_combo.set_sensitive(sensitive);
    m_refresh_button.set_sensitive(sensitive);
    m_flash_button.set_sensitive(sensitive);
}

void MainWindow::show_error(const std::string& message) {
    Gtk::MessageDialog dialog(*this, "Error", false, Gtk::MESSAGE_ERROR);
    dialog.set_secondary_text(message);
    dialog.run();
}

void MainWindow::show_info(const std::string& message) {
    Gtk::MessageDialog dialog(*this, "Success", false, Gtk::MESSAGE_INFO);
    dialog.set_secondary_text(message);
    dialog.run();
}