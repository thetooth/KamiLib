/** ou_thread.cpp
* implements the Thread class
* Author: Vijay Mathew Pandyalakal
* Date: 13-OCT-2003
**/

#include <string>
using namespace std;

#include <windows.h>

#include "threads.h"
namespace klib{

	const int KLGLThread::P_ABOVE_NORMAL = THREAD_PRIORITY_ABOVE_NORMAL;
	const int KLGLThread::P_BELOW_NORMAL = THREAD_PRIORITY_BELOW_NORMAL;
	const int KLGLThread::P_HIGHEST = THREAD_PRIORITY_HIGHEST;
	const int KLGLThread::P_IDLE = THREAD_PRIORITY_IDLE;
	const int KLGLThread::P_LOWEST = THREAD_PRIORITY_LOWEST;
	const int KLGLThread::P_NORMAL = THREAD_PRIORITY_NORMAL;
	const int KLGLThread::P_CRITICAL = THREAD_PRIORITY_TIME_CRITICAL;

	/**@ The Thread class implementation
	**@/

	/** Thread()
	* default constructor
	**/  
	KLGLThread::KLGLThread() {
		m_hThread = NULL;
		m_strName = "null";
	}

	/** Thread(const char* nm)
	* overloaded constructor
	* creates a Thread object identified by "nm"
	**/  
	KLGLThread::KLGLThread(const char* nm) {
		m_hThread = NULL;
		m_strName = nm;
	}

	KLGLThread::~KLGLThread() {
		if(m_hThread != NULL) {
			stop();
		}
	}

	/** setName(const char* nm)
	* sets the Thread object's name to "nm"
	**/  
	void KLGLThread::setName(const char* nm) {	
		m_strName = nm;
	}

	/** getName()
	* return the Thread object's name as a string
	**/  
	string KLGLThread::getName() const {	
		return m_strName;
	}

	unsigned long *KLGLThread::getHandle() {
		return m_hThread;
	}

	/** run()
	* called by the thread callback _ou_thread_proc()
	* to be overridden by child classes of Thread
	**/ 
	void KLGLThread::run() {
		// Base run
	}

	/** sleep(long ms)
	* holds back the thread's execution for
	* "ms" milliseconds
	**/ 
	void KLGLThread::sleep(long ms) {
		Sleep(ms);
	}

	/** start()
	* creates a low-level thread object and calls the
	* run() function
	**/ 
	void KLGLThread::start() {
		DWORD tid = 0;	
		m_hThread = (unsigned long*)CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)_ou_thread_proc,(KLGLThread*)this,0,&tid);
		if(m_hThread == NULL) {
			throw KLGLThreadException("Failed to create thread");
		}else {
			setPriority(KLGLThread::P_NORMAL);
		}
	}

	/** stop()
	* stops the running thread and frees the thread handle
	**/ 
	void KLGLThread::stop() {
		if(m_hThread == NULL) return;	
		WaitForSingleObject(m_hThread,INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	/** setPriority(int tp)
	* sets the priority of the thread to "tp"
	* "tp" must be a valid priority defined in the
	* Thread class
	**/ 
	void KLGLThread::setPriority(int tp) {
		if(m_hThread == NULL) {
			throw KLGLThreadException("Thread object is null");
		}else {
			if(SetThreadPriority(m_hThread,tp) == 0) {
				throw KLGLThreadException("Failed to set priority");
			}
		}
	}

	/** suspend()  
	* suspends the thread
	**/ 
	void KLGLThread::suspend() {
		if(m_hThread == NULL) {
			throw KLGLThreadException("Thread object is null");
		}else {
			if(SuspendThread(m_hThread) < 0) {
				throw KLGLThreadException("Failed to suspend thread");
			}
		}
	}

	/** resume()  
	* resumes a suspended thread
	**/ 
	void KLGLThread::resume() {
		if(m_hThread == NULL) {
			throw KLGLThreadException("Thread object is null");
		}else {
			if(ResumeThread(m_hThread) < 0) {
				throw KLGLThreadException("Failed to resume thread");
			}
		}
	}

	/** wait(const char* m,long ms)  
	* makes the thread suspend execution until the
	* mutex represented by "m" is released by another thread.
	* "ms" specifies a time-out for the wait operation.
	* "ms" defaults to 5000 milli-seconds
	**/ 
	bool KLGLThread::wait(const char* m,long ms) {
		HANDLE h = OpenMutex(MUTEX_ALL_ACCESS,FALSE,m);
		if(h == NULL) {
			throw KLGLThreadException("Mutex not found");
		}
		DWORD d = WaitForSingleObject(h,ms);
		switch(d) {
		case WAIT_ABANDONED:
			throw KLGLThreadException("Mutex not signaled");
			break;
		case WAIT_OBJECT_0:
			return true;
		case WAIT_TIMEOUT:
			throw KLGLThreadException("Wait timed out");
			break;
		}
		return false;
	}

	/** release(const char* m)  
	* releases the mutex "m" and makes it 
	* available for other threads
	**/ 
	void KLGLThread::release(const char* m) {
		HANDLE h = OpenMutex(MUTEX_ALL_ACCESS,FALSE,m);
		if(h == NULL) {
			throw KLGLThreadException("Invalid mutex handle");
		}
		if(ReleaseMutex(h) == 0) {
			throw KLGLThreadException("Failed to release mutex");
		}
	}

	/**@ The Mutex class implementation
	**@/

	/** Mutex()
	* default constructor
	**/  
	KLGLMutex::KLGLMutex() {
		m_hMutex = NULL;
		m_strName = "";
	}

	/** Mutex(const char* nm)
	* overloaded constructor
	* creates a Mutex object identified by "nm"
	**/  
	KLGLMutex::KLGLMutex(const char* nm) {	
		m_strName = nm;	
		m_hMutex = (unsigned long*)CreateMutex(NULL,FALSE,nm);
		if(m_hMutex == NULL) {
			throw KLGLThreadException("Failed to create mutex");
		}
	}

	/** create(const char* nm)
	* frees the current mutex handle.
	* creates a Mutex object identified by "nm"
	**/  
	void KLGLMutex::create(const char* nm) {
		if(m_hMutex != NULL) {
			CloseHandle(m_hMutex);
			m_hMutex = NULL;
		}
		m_strName = nm;
		m_hMutex = (unsigned long*)CreateMutex(NULL,FALSE,nm);
		if(m_hMutex == NULL) {
			throw KLGLThreadException("Failed to create mutex");
		}
	}
	/** getMutexHandle()
	* returns the handle of the low-level mutex object
	**/  
	unsigned long* KLGLMutex::getMutexHandle() {
		return m_hMutex;
	}

	/** getName()
	* returns the name of the mutex
	**/ 
	string KLGLMutex::getName() {
		return m_strName;
	}

	void KLGLMutex::release() {
		__try
		{
			if(m_hMutex != NULL) {
				ReleaseMutex(m_hMutex);
				CloseHandle(m_hMutex);
				m_hMutex = NULL;
			}
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			m_hMutex = NULL;
			printf("KLGLThreadException:: Failed to close mutex handle, it has been nullified to prevent further exceptions.\n");
		}
	}

	KLGLMutex::~KLGLMutex() {
		release();
	}

	// ThreadException
	KLGLThreadException::KLGLThreadException(const char* m) {
		msg = m;
		printf("KLGLThreadException:: %s\n", m);
	}

	string KLGLThreadException::getMessage() const {
		return msg;
	}

	// global thread caallback
	unsigned int _ou_thread_proc(void* param) {
		KLGLThread* tp = (KLGLThread*)param;
		tp->run();
		return 0;
	}
}