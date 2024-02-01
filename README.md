`AltBacktick` is a small Windows program that runs in background and allows to switch between windows of the same program using the keyboard shortcut `ALT`+`` ` `` (the `ALT` key can be configured).

![AltBacktick](./demo.gif)

It's similar to [Easy Window Switcher](https://neosmart.net/EasySwitch/) but it supports more programs (for instance TablePlus), handles minimized windows and is open source.

*The key `` ` `` (`backtick`) is above the `TAB` key on QWERTY keyboard but even if your keyboard is not QWERTY, the shortcut will be mapped to the key above the `TAB` key.*

## Usage

### Installing

1. Download the last executable from [GitHub](https://github.com/akiver/AltBacktick/releases) or clone the repository (`git clone --recursive https://github.com/akiver/AltBacktick`) and build it from Visual Studio
2. Run the program
3. Click on `Automatically start AltBacktick` (or `Run AltBacktick without installing` if you only want to run it)

### Uninstalling

1. Run the program
2. Select `Uninstall AltBacktick`

### Configuring

When the program starts, it tries to read a possible `INI` configuration file located at `~/.altbacktick`.  
This file allows to configure the program and must be created manually if default values are not suitable.

Example:

```ini
[keyboard]
modifier_key=ctrl
```

**If you edit the configuration file while the program is running, you need to kill the program process and restart it!**

#### modifier_key

The `modifier_key` option allows to change the modifier key that needs to be pressed to switch between windows.  
Possible values are `alt` and `ctrl`, default is `alt`.

## License

[GPL v2](https://github.com/akiver/AltBacktick/blob/master/LICENSE)
