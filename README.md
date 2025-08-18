## What is this?
This is Firmware plus wiring for a portable stereo recorder using a ESP32-S2 & PCM1808

Why? I want to make a portable binaural mic but.. maybe instead of a weird stand, you use your ears! (tbh idk if thatll work.)
Later on I plan to make a portable ambisonic mic setup, using a ESP32-S3 & 2x PCM1808. This is just a test to see if stuff works.


As for rn, well heres how the audio works:
<img width="1527" height="276" alt="image" src="https://github.com/user-attachments/assets/af503a79-f880-4826-bc1b-e8100e658d07" />

You can see theres noise, Might add some filteres or who knows what, I put a function generator and manually switching between L & R channel.

<img width="1210" height="702" alt="image" src="https://github.com/user-attachments/assets/c59b78e5-7020-43ba-b3b4-d76143576335" />

<img width="727" height="888" alt="image" src="https://github.com/user-attachments/assets/0d57b3e8-a55b-4d62-a18a-8726e20a9990" />

## Wiring

##### SD

SD_CS 38

SD_MOSI 35

SD_MISO 36

SD_SCK 37

3V3 to 3.3V


##### PCM1808

1808_SCK 9

1808_BCK 3

1808_LRC 7

1808_OUT 5

FMY, MD1, MD0, GND - To GND

Connect +5V to VBUS/5V

Connect 3.3V to 3.3V

**Note: Both 3v and 5v need to be connected at once on the PCM1808**
