# Basalt Shell (bsh) v0.x

## Goals
- Friendly, discoverable CLI
- Simple commands for non-engineers
- Scriptable but not POSIX-heavy

## Core commands
- help: list commands
- ls [path]: list files
- cd <path>: change directory
- pwd: print working directory
- cat <path>: view file
- edit <path>: minimal line editor (ends with .save or .quit)
- mkdir <path>: create a directory
- cp [-r|-R] <src> <dst>: copy file or directory
- mv [-r|-R] <src> <dst>: move/rename file or directory
- rm <path>: delete a file
- rm -r <path>: delete a directory recursively
- run <app|path>: run an app or script
- stop: stop the currently running app
- kill: force-stop the currently running app
- apps: list installed apps
- install <src> [name]: install from folder or store-only zip
- remove <app>: uninstall an app
- logs: show runtime diagnostics (running app, last runtime error, heap/cpu info)
- wifi [status|scan|connect|reconnect|disconnect]: Wi-Fi station tools
- reboot: restart device

## Examples

List root and SD:
```
ls /
ls /sd
```

Install a zip from SD:
```
install /sd/blink_rgb.zip blink_rgb
```

Copy an app to SD:
```
mkdir /sd/apps
cp /apps/demo /sd/apps/demo
```

Remove a directory:
```
rm -r /sd/.Trash-1000
```

Move a directory:
```
mv -r /sd/apps/demo /sd/apps/demo_backup
```

Move an app prefix (SPIFFS flat layout):
```
mv /apps/demo /sd/apps/demo
```
