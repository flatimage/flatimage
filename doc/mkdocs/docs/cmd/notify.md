# Startup Notification

## What is it?

The `fim-notify` command configures FlatImage to display a desktop notification every time the container starts. This provides visual feedback that the application is launching, particularly useful for applications with longer startup times or those running in the background.

<p align="center">
  <img src="https://raw.githubusercontent.com/flatimage/docs/master/docs/image/notify.png" />
</p>

**Use Cases:**

- Provide user feedback for GUI applications with slow startup
- Confirm background services have started successfully
- Show application information at launch (name, version)
- Brand your application with custom notifications
- Notify users when multiple instances are started

## How to Use

You can use `./app.flatimage fim-help notify` to get the following usage details:

```txt
fim-notify : Notify with 'notify-send' when the program starts
Usage: fim-notify <on|off>
  <on> : Turns on notify-send to signal the application start
  <off> : Turns off notify-send to signal the application start
```

### Enable Startup Notification

Activate desktop notifications on launch:

```bash
./app.flatimage fim-notify on
```

Now every time you execute the FlatImage, a desktop notification appears confirming the application is starting.

**Example:**
```bash
# Enable notifications
./app.flatimage fim-notify on

# Run the application
./app.flatimage
# → Desktop notification appears: "Starting MyApp"
```

### Disable Startup Notification

Turn off startup notifications:

```bash
./app.flatimage fim-notify off
```

The FlatImage will launch silently without displaying notifications.

**Example:**
```bash
# Disable notifications
./app.flatimage fim-notify off

# Run the application
./app.flatimage
# → No notification displayed
```

## How it Works

Startup notifications use the `notify-send` command-line tool, which is part of the `libnotify` library and supported by most modern Linux desktop environments.

**Configuration Storage:**

The notification setting is stored in the FlatImage binary as a boolean flag. This configuration persists across executions and layer commits.