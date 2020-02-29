# Centralina
Emulazione di un sistema di domotica, progetto d'esame di Sistemi Operativi 1.
[Leggi il compito assegnato per ulteriori informazioni](LabSO--2018_2019--PROGETTI_I.pdf)

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

  1. **Clone** this repo on your host using [Git](https://git-scm.com)

     ```console
     $ git clone https://github.com/giovannirizzi/centralina.git
     ```

  2. **Change** current working **directory**

     ```console
     $ cd centralina/project
     ```
  3. **Compile** all the executables with [GNU Make](https://www.gnu.org/software/make/)

     ```console
     $ make build
     ```

## Run
All binaries are under `./bin/` folder

  1. **controller**

     > Used to control all the devices automatically and manage the entire system.

     ```console
     $ ./bin/controller
     ```

  2. **manualcontrol**

     > Used to simulate manual control of devices.

     ```console
     $ ./bin/manualcontrol
     ```

## Commands
- ### Controller
```
Usage: controller
available commands: 
    - list: show the list of available and active devices
            usage: <list>
    - add: add a new device to the system
            usage: <add> <device>
    - del: remove the identified device from the system
            it also remove connected devices, if its a control device
            usage: <del> <id>
    - link: connect the first device to the second
            usage: <link> <id> <id>
    - switch: turn on/off the related switch of the device 
            usage: <switch> <id> <switch name> <on/off>
    - set: set the register of the identified device 
            usage: <set> <id> <register> <value>
    - info: show details of the identified device 
            usage: <info> <id>
    - exit: close the controller
            usage: <exit>
```

-  ### Manualcontrol
```
Usage: manualcontrol [COMMAND] [ARGS]
available commands: 
    - set: set value of the identified device
            usage: <set> <id> <registry> <value>
    - whois: return pid of the identified device
            usage: <whois> <id>
    - switch: turn on/off the identified device
            usage: <switch> <id> <label> <on/off>
    - help: show available commands
            usage: <help>
```

## Authors
- **Giovanni Rizzi**
- **Omar Battan**
- **Marco Menapace**

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details

&copy; Centralina 2020
