#include "sound.h"

namespace klib{
	// Internal WaveOut API callback function. We just call our sound handler ("playNextBuffer")
	static void CALLBACK waveOutProc(HWAVEOUT hwo,UINT uMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2)
	{
		if (uMsg == WOM_DONE)
		{
			KLGLSound *pServer = (KLGLSound*)dwInstance;
			//if (pServer) pServer->fillNextBuffer();
		}
	}

	KLGLSound::KLGLSound()
	{
		m_pUserCallback = NULL;
		m_bServerRunning = FALSE;
		m_currentBuffer = 0;
	}

	KLGLSound::~KLGLSound()
	{
		close();
	}

	bool KLGLSound::open(KLGLSound_CALLBACK pUserCallback,long totalBufferedSoundLen)
	{

		m_pUserCallback = pUserCallback;
		m_bufferSize = ((totalBufferedSoundLen * APP_REPLAY_RATE) / 1000) * APP_REPLAY_SAMPLELEN;
		m_bufferSize /= APP_REPLAY_NBSOUNDBUFFER;

		WAVEFORMATEX	wfx;
		wfx.wFormatTag = 1;		// PCM standard.
		wfx.nChannels = 1;		// Mono
		wfx.nSamplesPerSec = APP_REPLAY_RATE;
		wfx.nAvgBytesPerSec = APP_REPLAY_RATE*APP_REPLAY_SAMPLELEN;
		wfx.nBlockAlign = APP_REPLAY_SAMPLELEN;
		wfx.wBitsPerSample = APP_REPLAY_DEPTH;
		wfx.cbSize = 0;
		MMRESULT errCode = waveOutOpen(	&m_hWaveOut,
			WAVE_MAPPER,
			&wfx,
			(DWORD)waveOutProc,
			(DWORD)this,					// User data.
			(DWORD)CALLBACK_FUNCTION);

		if (errCode != MMSYSERR_NOERROR) return FALSE;

		// Alloc the sample buffers.
		for (int i=0;i<APP_REPLAY_NBSOUNDBUFFER;i++)
		{
			m_pSoundBuffer[i] = malloc(m_bufferSize);
			memset(&m_waveHeader[i],0,sizeof(WAVEHDR));
		}

		// Fill all the sound buffers
		m_currentBuffer = 0;
		for (int i = 0; i < APP_REPLAY_NBSOUNDBUFFER; i++)
			fillNextBuffer();

		m_bServerRunning = TRUE;
		return TRUE;
	}

	void KLGLSound::close(void)
	{

		if (m_bServerRunning)
		{
			m_pUserCallback = NULL;
			waveOutReset(m_hWaveOut);					// Reset tout.
			for (int i=0;i<APP_REPLAY_NBSOUNDBUFFER;i++)
			{
				if (m_waveHeader[m_currentBuffer].dwFlags & WHDR_PREPARED)
					waveOutUnprepareHeader(m_hWaveOut,&m_waveHeader[i],sizeof(WAVEHDR));

				free(m_pSoundBuffer[i]);
			}
			waveOutClose(m_hWaveOut);
			m_bServerRunning = FALSE;
		}
	}

	void KLGLSound::fillNextBuffer(void)
	{

		// check if the buffer is already prepared (should not !)
		if (m_waveHeader[m_currentBuffer].dwFlags&WHDR_PREPARED)
			waveOutUnprepareHeader(m_hWaveOut,&m_waveHeader[m_currentBuffer],sizeof(WAVEHDR));

		// Call the user function to fill the buffer with anything you want ! :-)
		if (m_pUserCallback)
			m_pUserCallback(m_pSoundBuffer[m_currentBuffer],m_bufferSize);

		// Prepare the buffer to be sent to the WaveOut API
		m_waveHeader[m_currentBuffer].lpData = (char*)m_pSoundBuffer[m_currentBuffer];
		m_waveHeader[m_currentBuffer].dwBufferLength = m_bufferSize;
		waveOutPrepareHeader(m_hWaveOut,&m_waveHeader[m_currentBuffer],sizeof(WAVEHDR));

		// Send the buffer the the WaveOut queue
		waveOutWrite(m_hWaveOut,&m_waveHeader[m_currentBuffer],sizeof(WAVEHDR));

		m_currentBuffer++;
		if (m_currentBuffer >= APP_REPLAY_NBSOUNDBUFFER) m_currentBuffer = 0;
	}
}