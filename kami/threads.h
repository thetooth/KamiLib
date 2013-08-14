/** ou_thread.h
* declares the Thread interface and associated classes
* Author: Vijay Mathew Pandyalakal
* Date: 13-OCT-2003
**/

#pragma once

#include <string>

namespace klib {
	using namespace std;
	class KLGLThreadException;
	class KLGLMutex;

	/** class Thread
	* Represents a thread of execution
	* in the process. To create a new Thread
	* write an inherited class ot Thread and
	* override the run() function
	**/
	class KLGLThread {
	public:
		KLGLThread();
		KLGLThread(const char* nm);
		virtual ~KLGLThread();
		virtual void setName(const char* nm);
		virtual string getName() const;
		virtual unsigned long *getHandle();
		virtual void start();
		virtual void run();
		virtual void sleep(long ms);
		virtual void suspend();
		virtual void resume();
		virtual void stop();

		virtual void setPriority(int p);

		virtual bool wait(const char* m,long ms=5000);
		virtual void release(const char* m);

		// unsigned long* to the low-level thread object
		unsigned long* m_hThread;
		// a name to identify the thread
		string m_strName;
		// Thread priorities
		static const int P_ABOVE_NORMAL;
		static const int P_BELOW_NORMAL;
		static const int P_HIGHEST;
		static const int P_IDLE;
		static const int P_LOWEST;
		static const int P_NORMAL;
		static const int P_CRITICAL;			
	};// class Thread	

	/** class Mutex
	* Represents a Mutex object to synchronize
	* access to shaed resources.
	**/
	class KLGLMutex {
	public:
		// unsigned long* to the low-level mutex object
		unsigned long* m_hMutex;
		// name to identify the mutex
		string m_strName;
		KLGLMutex();
		KLGLMutex(const char* nm);
		void create(const char* nm);
		unsigned long* getMutexHandle();
		string getName();
		void release();
		~KLGLMutex();
	};

	/** class ThreadException
	* thrown by Thread and Mutex function 
	* calls
	**/
	class KLGLThreadException {
	private:
		string msg;
	public:
		KLGLThreadException(const char* m);
		string getMessage() const;
	};	

	// global function called by the thread object.
	// this in turn calls the overridden run()
	extern "C" {
		unsigned int _ou_thread_proc(void* param);
	}
}