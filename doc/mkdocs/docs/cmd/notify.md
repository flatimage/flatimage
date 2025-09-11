# Configure startup notification

## What is it?

This is a facility to display a notification every time a FlatImage program starts. It uses `notify-send` and displays the notification with the application icon if the desktop integration is configured.

<p align="center">
  <img src="https://raw.githubusercontent.com/flatimage/docs/master/docs/image/notify.png" />
</p>

## How to use

You can use `./app.flatimage fim-help notify` to get the following usage details:

```txt
fim-notify : Notify with 'notify-send' when the program starts
Usage: fim-notify <on|off>
  <on> : Turns on notify-send to signal the application start
  <off> : Turns off notify-send to signal the application start
```

To enable the startup notification:

```bash
./app.flatimage fim-notify on
```

To disable the startup notification:

```bash
./app.flatimage fim-notify off
```

## How it works

FlatImage uses the `notify-send` command with the `-i` option to specify the application icon.