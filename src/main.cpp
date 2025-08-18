#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <driver/i2s.h>

#define BUTTON_PIN 0
#define LED_PIN 15

#define SD_CS 38
#define SD_MOSI 35
#define SD_MISO 36
#define SD_SCK 37
SPIClass sdSPI(FSPI);

#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 48000
#define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_32BIT
#define CHANNEL_FORMAT I2S_CHANNEL_FMT_RIGHT_LEFT

#define BUFFER_SIZE 4096

File file;
bool recording = false;
uint32_t bytesWritten = 0;

struct WAVHeader
{
  char riff[4];
  uint32_t size;
  char wave[4];
  char fmt[4];
  uint32_t fmt_size;
  uint16_t audio_fmt;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
  char data[4];
  uint32_t data_size;
};

void writeWavHeader(File &f, uint32_t pcm_bytes)
{
  WAVHeader h;
  memcpy(h.riff, "RIFF", 4);
  h.size = pcm_bytes + 36;
  memcpy(h.wave, "WAVE", 4);
  memcpy(h.fmt, "fmt ", 4);
  h.fmt_size = 16;
  h.audio_fmt = 1;
  h.num_channels = 2;
  h.sample_rate = SAMPLE_RATE;
  h.bits_per_sample = 24;
  h.byte_rate = SAMPLE_RATE * h.num_channels * (h.bits_per_sample / 8);
  h.block_align = h.num_channels * (h.bits_per_sample / 8);
  memcpy(h.data, "data", 4);
  h.data_size = pcm_bytes;
  f.seek(0);
  f.write((uint8_t *)&h, sizeof(h));
}

void i2sInit()
{
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = true,
      .tx_desc_auto_clear = false};

  i2s_pin_config_t pin_config = {
      .mck_io_num = GPIO_NUM_9, // SCK
      .bck_io_num = GPIO_NUM_3, // BCK
      .ws_io_num = GPIO_NUM_7,  // LRC
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = GPIO_NUM_5 // OUT
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
  i2s_set_clk(I2S_PORT, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_32BIT, I2S_CHANNEL_STEREO);
  i2s_zero_dma_buffer(I2S_PORT);
}

int getNextFileNumber()
{
  int num = 1;
  while (SD.exists("/record" + String(num) + ".wav"))
  {
    num++;
  }
  return num;
}

void setup()
{
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, sdSPI))
  {
    Serial.println("SD Card init failed!");

    unsigned long startTime = millis();
    while (millis() - startTime < 5000)
    {
      digitalWrite(LED_PIN, HIGH);
      delay(250);
      digitalWrite(LED_PIN, LOW);
      delay(250);
    }

    while (1)
      ;
  }

  Serial.println("SD card ready.");

  i2sInit();
}

void loop()
{
  static uint8_t buffer[BUFFER_SIZE];
  static bool lastButtonState = HIGH;

  bool buttonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && buttonState == LOW)
  {
    if (!recording)
    {
      int fileNum = getNextFileNumber();
      String filename = "/record" + String(fileNum) + ".wav";
      file = SD.open(filename, FILE_WRITE);
      if (!file)
      {
        Serial.println("Did you remove your SD card?");

        unsigned long startTime = millis();
        while (millis() - startTime < 5000)
        {
          digitalWrite(LED_PIN, HIGH);
          delay(250);
          digitalWrite(LED_PIN, LOW);
          delay(250);
        }

        return;
      }
      WAVHeader blank = {0};
      file.write((uint8_t *)&blank, sizeof(blank));
      bytesWritten = 0;
      recording = true;
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Recording...");
    }
    else
    {
      Serial.println("Saved!");
      writeWavHeader(file, bytesWritten);
      file.close();
      recording = false;
      digitalWrite(LED_PIN, LOW);
    }
    delay(200);
  }

  lastButtonState = buttonState;

  if (recording)
  {
    size_t bytesRead = 0;
    i2s_read(I2S_PORT, buffer, BUFFER_SIZE, &bytesRead, portMAX_DELAY);

    if (bytesRead > 0)
    {
      int32_t *raw32 = (int32_t *)buffer;
      size_t samples = bytesRead / 4;

      uint8_t packed[BUFFER_SIZE * 3 / 4];

      for (size_t i = 0; i < samples; i++)
      {
        int32_t s = raw32[i];
        packed[i * 3 + 0] = (s >> 8) & 0xFF;
        packed[i * 3 + 1] = (s >> 16) & 0xFF;
        packed[i * 3 + 2] = (s >> 24) & 0xFF;
      }

      size_t packedBytes = samples * 3;
      file.write(packed, packedBytes);
      bytesWritten += packedBytes;
    }

    static unsigned long lastFlush = 0;
    if (millis() - lastFlush > 1000)
    {
      file.flush();
      lastFlush = millis();
    }
  }
}
