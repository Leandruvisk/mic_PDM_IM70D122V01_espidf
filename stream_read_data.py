import serial
import numpy as np
import sounddevice as sd

PORT = "/dev/ttyACM0"   # ajuste
BAUD = 2000000#921600
SAMPLE_RATE = 48000
CHUNK = 1024

ser = serial.Serial(PORT, BAUD)

stream = sd.OutputStream(
    samplerate=SAMPLE_RATE,
    channels=1,
    dtype='int16',
    blocksize=CHUNK
)

stream.start()

print("Streaming...")

while True:
    raw = ser.read(CHUNK * 2)  # 2 bytes por sample

    if len(raw) == CHUNK * 2:
        audio = np.frombuffer(raw, dtype=np.int16).astype(np.int32)

        # remover DC
        audio = audio - np.mean(audio)

        # filtro passa-alta simples
        alpha = 0.995
        hp = np.zeros_like(audio)
        for i in range(1, len(audio)):
            hp[i] = alpha * (hp[i-1] + audio[i] - audio[i-1])

        audio = np.clip(hp, -32768, 32767).astype(np.int16)

        stream.write(audio)