# Binding

It is possible to bind applications from the host to the container, this is not recommended
because of issues with `glibc` and undefined symbols.

## MangoHud

Example with `arch.flatimage`:

```
# Set permissions
./arch.flatimage fim-perms add audio,wayland,xorg,gpu,network
# Install drivers
./arch.flatimage fim-root pacman -S --noconfirm vulkan-tools xf86-video-amdgpu vulkan-radeon lib32-vulkan-radeon xf86-video-intel vulkan-intel lib32-vulkan-intel
# Bind mangohud vulkan layers
./arch.flatimage fim-bind add ro /usr/share/vulkan /usr/share/vulkan
# Bind mangohud libraries
./arch.flatimage fim-bind add ro /usr/lib/mangohud /usr/lib/mangohud
# Bind mangohud binaries
./arch.flatimage fim-bind add ro /usr/bin /usr/local/bin
# Update PATH
./arch.flatimage fim-env set 'PATH=$PATH:/usr/local/bind'
# Test the mangohud installation of the host, from inside the container
./arch.flatimage fim-exec mangohud vkcube
```

