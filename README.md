# 🌃 fmtx_qn80xx 🌃

![GitHub license](https://img.shields.io/github/license/jensrene/fmtx_qn80xx?style=plastic)
![GitHub issues](https://img.shields.io/github/issues-raw/jensrene/fmtx_qn80xx?color=red&style=plastic)
![GitHub stars](https://img.shields.io/github/stars/jensrene/fmtx_qn80xx?color=blue&style=plastic)

<div align="center">
    <img src="https://path-to-your-image.jpg" alt="fmtx_qn80xx image">
</div>

## 💽 About the Project

Welcome to the fmtx_qn80xx project. This project is for controlling qn80xx (for example qn8066) fm transmitter chips via i2c bus. It is especially build for cheap chinese fm transmitter boards, like [this one](https://de.aliexpress.com/item/33002132211.html) sold on dealers like aliexpress. Similar looking boards exist, so check for QN8066!

Here's a link to our [Wiki](https://github.com/jensrene/fmtx_qn80xx/wiki) for more details (currently still empty)

## 🚀 Getting Started

To get started with the project, you need a raspberry pi with GPIO header and connect that to the connector on the fm transmitter board, on which normally the display (and µC) board is connected to. 

## 💾 Installation
Connect the most left (usually brown) pin 1 of the fm transmitter board (3.3V) to pin 1 on the raspberry pi (3.3V)
Connect the next pin 2 (usually red) to pin 9 on the pi (GND)
Connect pin 3 (usually orange) to pin 3 on the pi (I2C-SDA)
Connect pin 4 (usually yellow) to pin 5 on the pi (I2C-SCL)
Connect pin 5, the most right one (usually green) to pin 12 on the pi (PWM)

## ⚙️ Usage
```
./fmtx [options]

Usage:
  ./fmtx -r <raw_command> | -c <config_file> | -e | -d | -s | -a | -p <power_value> | -q

Options:
  -r, --raw-command <raw_command>    Send a raw command to the program. Valid raw commands include:
                                     stop, activate, reset, status, rdson, rdsoff, power=<value>.
                                     'power' value must be between 0 and 103.
  -c, --config <config_file>         Load program configurations from a specified file.
  -e, --execute                      Start listener, output to stdout but don't daemonize.
  -d, --daemon                       Start the program as a daemon listener.
  -s, --status                       Gets the status of the transmitter from a running daemon.
  -a, --activate                     Activates the transmission of fm with configured settings.
  -p, --power <power_value>          Sets the power of the transmitter, basially voltage to the '.
                                     power transistor. 'power' value must be between 0 and 103.
  -q, --stop                         Stops the transmission and the daemon exits.

Please note:
1. The program will execute the options in the order they are given. If multiple raw commands are given, only the last one will take effect.
2. When using the --config option, the program will exit if the specified configuration file does not exist.
3. When using the --daemon option, the program will run in the background as a daemon.
4. The --execute option will start the listener and output to stdout, but will not daemonize the program.
```

For PWM, run the program as root. (still working on that)


## 🎓 Contributing

Currently, if you really want to, just write me a message/email :) But I don't expect much interest ;)

## 📖 License

This project is licensed under [GNU GPL v3.0](https://www.gnu.org/licenses/gpl-3.0.html.en) - see the LICENSE.md file for details.

## 📫 Contact

Jens-René Wiedemann - jensrene@lionpaws.de

Project Link: [https://github.com/jensrene/fmtx_qn80xx](https://github.com/jensrene/fmtx_qn80xx)
