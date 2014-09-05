minisvr.c:tcp server,select,single proc
minisvr_mt.c:tcp server,select,double proc
minisvr_mt2.c:tcp server,select,multi proc for each conn
2014.09.05:
	增加signal处理，忽略SIGCHLD，让系统来回收提前成功退出的子进程（退出失败的子进程默认都会由系统回收资源）。如果没有忽略SIGCHLD，那主进程就要调用wait()回收子进程资源