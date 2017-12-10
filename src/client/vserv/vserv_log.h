#ifndef _VSERV_LOG_H_
#define _VSERV_LOG_H_

#define GS_LOG_GET(PREFIX) NULL

struct GsLogBase;

class GsLogGuard {
public:
	GsLogGuard(GsLogBase *Log)
		: mLog(Log)
	{
	}

	~GsLogGuard() {
	}

	GsLogBase *GetLog() {
		return mLog;
	}

private:
	GsLogBase *mLog;
};

typedef GsLogGuard log_guard_t;

#endif /* _VSERV_LOG_H_ */
