![Shannon screenshot](https://github.com/SuperFromND/b/assets/22881403/a3a229d0-16a0-424f-abc7-fe0b70763245)

---
**Shannon** is a basic frontend for the [touchHLE](https://touchhle.org/) emulator.

I created this launcher as touchHLE's current frontend does not allow for more than 16 apps to be displayed, and I had difficulty setting up a Rust enviroment to add pagination support to touchHLE directly. This was made mostly for my personal use, and as a result, it only supports Windows at the moment.
### **Shannon has not been widely tested and may contain security bugs. Use at your own risk.**
# Installing
[Download the release](https://github.com/SuperFromND/shannon/releases/latest/download/shannon-windows.zip), then extract the contents of the ZIP to the same directory that touchHLE's executable is located in. Double-click and Shannon should open, displaying a list of all apps in the `touchHLE_apps` directory. Navigate the list using the scroll wheel and click a given file to launch it in touchHLE.

Note that touchHLE is in a very early stage of developement right now, so the vast majority of apps will close nearly instantly. Check [their compatiability list](https://github.com/hikari-no-yume/touchHLE/blob/trunk/APP_SUPPORT.md) for known good apps.
# Building
You should be able to compile this pretty easily as long as you have SDL2 and a C++ compiler ready to go.
```
git clone https://github.com/SuperFromND/shannon.git
cd shannon
make
make install
```
# Licensing
Shannon's source code is [available under the MIT License.](https://raw.githubusercontent.com/SuperFromND/example/master/LICENSE) <3