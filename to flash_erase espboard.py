import esptool

# Replace with your correct COM port!
port = "COM6"  
baud = 115200

print(f"Erasing ESP32 flash on {port}...")

# Arguments like command-line: esptool.py --port COM6 erase_flash
esptool.main([
    "--chip", "esp32",
    "--port", port,
    "--baud", str(baud),
    "erase_flash"
])
