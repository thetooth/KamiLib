#pragma once

#include <Windows.h>
#include <MMSystem.h>
#include <stdlib.h>

namespace klib{

#define	APP_REPLAY_RATE				44100
#define	APP_REPLAY_DEPTH			16
#define	APP_REPLAY_SAMPLELEN		(APP_REPLAY_DEPTH/8)
#define	APP_REPLAY_NBSOUNDBUFFER	2

	typedef void (*KLGLSound_CALLBACK) (void *pBuffer,long bufferLen);

	class KLGLSound {
	public:
		KLGLSound();
		~KLGLSound();

		bool open(KLGLSound_CALLBACK pUserCallback, long totalBufferedSoundLen=4000); // Buffered sound, in ms
		void close(void);

		// Do *NOT* call this internal function:
		void fillNextBuffer(void);
	private:
		bool m_bServerRunning;
		HWND m_hWnd;
		long m_bufferSize;
		long m_currentBuffer;
		HWAVEOUT m_hWaveOut;
		WAVEHDR m_waveHeader[APP_REPLAY_NBSOUNDBUFFER];
		void *m_pSoundBuffer[APP_REPLAY_NBSOUNDBUFFER];
		KLGLSound_CALLBACK m_pUserCallback;
	};
}