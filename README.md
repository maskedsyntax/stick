<p align="center">
  <img src="assets/stick.png" alt="Stick Logo" width="150" height="150" />
</p>

# Stick

Stick is a minimal Linux-only GUI for writing ISO images to USB drives.

It provides a straightforward interface to select an ISO file and a target USB drive, handling the flashing process securely in the background. It is designed to be a lightweight and reliable alternative for users who frequently switch distributions.

## Installation

### Option 1: Debian/Ubuntu Package (.deb)

The easiest way to install Stick is by downloading the latest `.deb` package from the [Releases](https://github.com/maskedsyntax/stick/releases) page.

1. Download the `stick_x.x.x_amd64.deb` file.
2. Install it using `apt` or `dpkg`:

```bash
sudo apt install ./stick_*.deb
```

### Option 2: Build from Source

To build Stick from source, you need a C++ compiler and the GTKmm development libraries.

#### Dependencies

Ensure you have the following packages installed:

*   `build-essential` (Make, G++, etc.)
*   `libgtkmm-3.0-dev`
*   `pkg-config`

**Runtime Dependencies:**
*   `pkexec`
*   `coreutils` (dd)
*   `util-linux` (lsblk)

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential libgtkmm-3.0-dev pkg-config
```

#### Build Steps

1. Clone the repository:
```bash
git clone https://github.com/maskedsyntax/stick.git
cd stick
```

2. Compile the application:
```bash
make
```

3. Install (optional, for system integration):
```bash
sudo make install
```

4. Run the application:
```bash
stick
```
(Or run directly from build folder: `./build/stick`)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.