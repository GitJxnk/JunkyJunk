# KoGaMa Feed Monitor

An enchanced rewrite of the previous [monitor](https://github.com/GitJxnk/KGM-Junk/tree/main/Tools/MonitorAPI) made in python, inspired by [Devork's](https://github.com/Devorkk/) original idea. <br>
Monitors all existing posts & feeds on the website by tracking the latest ID changes

##  Features

-  **Real-time monitoring** – Detects status updates, wall posts, game publishes, marketplace purchases, badges earned, and username changes
-  **Auto gap detection** – Automatically skips missing feed IDs and jumps to the next valid event, this was a critical solution that was never ever implemented in the older version.
-  **Rate limit handling** – Built-in retry logic with exponential backoff for Discord API limits
-  **Persistent progress** – Saves last processed ID to `last_id.txt` for seamless restarts
- 🐧 **Cross-platform** – Runs on Windows and Linux

##  Requirements

- C++17 compiler (g++ or MSVC)
- libcurl
- nlohmann/json (header-only library)
- For Windows only (MSYS2/MinGW)

## Setup

### [🐧] Linux Distributions

```bash
sudo apt install libcurl4-openssl-dev g++
git clone https://github.com/GitJxnk/KGM-Junk/tree/main/Tools/PostsMonitor
cd PostsMonitor
g++ main.cpp -std=c++17 -lcurl -o monitor
```

### Arch Linux

```bash
sudo pacman -S curl gcc
```

### Fedora/RHEL

```bash
sudo dnf install libcurl-devel gcc-c++
```

### Windows

```bash
pacman -S mingw-w64-x86_64-curl mingw-w64-x86_64-gcc
g++ main.cpp -std=c++17 -I"C:\libs\curl\include" -I. -L"C:\libs\curl\lib" -lcurl -lws2_32 -lcrypt32 -o monitor.exe
```
**Disclaimer** : The software was tested only in an Arch Linux x64 build, as for other Linux distributions, there is no data given.

## Configuration

- **Creating a webhook** : Create a Discord webhook in your server settings (Server Settings → Integrations → Webhooks)
- Paste your Webhook URL in the string

```std::string WEBHOOK = "YOUR_WEBHOOK_URL_HERE";```

## Compiler Configurations

Create a ``.vscode`` folder and create a file named  ``c_cpp_properties.json`` Inside the folder, paste the following.

```json
// Windows configurations

{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/curl/include"
            ],
            "compilerPath": "C:/msys64/ucrt64/bin/g++.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-gcc-x64"
        }
    ],
    "version": 4
}
```

```json
// Linux configurations

{
    "configurations": [
        {
    "name": "Linux",
    "includePath": [
        "${workspaceFolder}/**",
        "${workspaceFolder}/curl/include"
    ],
    "compilerPath": "/usr/bin/g++",
    "cStandard": "c17",
    "cppStandard": "c++17",
    "intelliSenseMode": "linux-gcc-x64"
        }
    ],
    "version": 4
}
```

## Usage

```bash
./monitor - Linux
monitor.exe - Windows
```
The monitor will:

- Start from the last saved ID (or fallback to a default)

- Process all feed events sequentially

- Send notifications to your Discord channel

Pressing Ctrl+C will close the Terminal.

## How It Works

Polls https://www.kogama.com/api/feed/0/{ID}/ for each feed ID

- When a 404 is encountered, scans ahead up to 200 IDs to find the next valid event

- Detects true end by counting consecutive gaps (threshold: 10)

- At true end, polls every 10 seconds for new events

- Updates last_id.txt after each successfully processed event

## License

Check the repository main [License](https://github.com/GitJxnk/KGM-Junk?tab=License-1-ov-file)

## Credits & honorable mentions

- Massive thanks to [Zode](https://github.com/Zode) for providing informations and suggesting the method **Blind Guesser.** <br>
- [Devork](https://github.com/Devorkk/) For proof of concept and inspiration.
