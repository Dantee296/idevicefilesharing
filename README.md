
`idevicefilesharing` is a command line tool to manage files in the file sharing folder of an iOS app installed on a device.

Usage
=====
- `idevicefilesharing` help text

```
Usage: idevicefilesharing [OPTIONS] COMMAND
Manage App file sharing for idevice.

 Where COMMAND is one of:
  list_apps             list installed apps with file sharing enabled.
       list APP_ID      list of files in the apps file sharing directory.
        put APP_ID FILE [..] copy files into the file sharing folder of the app
     delete APP_ID FILE [..] delete files from the app file sharing folder

 The following OPTIONS are accepted:
  -u, --udid UDID  target specific device by its 40-digit device UDID
  -h, --help       prints usage information

```

- list apps supporting file sharing, with there bundle ids
```bash
$ idevicefilesharing list_apps
```
- list files in the file sharing folder of an app
```bash
$ idevicefilesharing list <bundle id>
```
- copy file to file sharing folder of an app
```bash
$ idevicefilesharing put <bundle id>  <file path>
```
- delete a file inside file sharing folder
```bash
$ idevicefilesharing delete <bundle id> <file name>
```

Requirements
============
Dev packages of:
- [`libimobiledevice`](https://github.com/libimobiledevice/libimobiledevice)
- [`libplist`](https://github.com/libimobiledevice/libplist)

Installation
============
To Compile and install:
```bash
    $ ./autogen.sh
    $ make
    $ make install
```

TODO
====
- support wildcard for filenames
- support copy from device 
- support folder get and put
- add interactive prompt like `sftp`
