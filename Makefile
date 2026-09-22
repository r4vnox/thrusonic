CC = gcc
CFLAGS = -Wall -O2
ALSA = -lasound -lm
FFTW = -lfftw3 -lm

all: tsonic-send tsonic-recv tsonic-rec tsonic-fft encoder generator

tsonic-send: tsonic-send.c
	$(CC) $(CFLAGS) $< -o $@ $(ALSA)

tsonic-recv: tsonic-recv.c
	$(CC) $(CFLAGS) $< -o $@ $(FFTW)

tsonic-rec: tsonic-rec.c
	$(CC) $(CFLAGS) $< -o $@ $(ALSA)

tsonic-fft: tsonic-fft.c
	$(CC) $(CFLAGS) $< -o $@ $(FFTW)

encoder: encoder.c
	$(CC) $(CFLAGS) $< -o $@ -lm

generator: generator.c
	$(CC) $(CFLAGS) $< -o $@ $(ALSA)

clean:
	rm -f tsonic-send tsonic-recv tsonic-rec tsonic-fft encoder generator

.PHONY: all clean
