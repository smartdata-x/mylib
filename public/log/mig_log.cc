#include "mig_log.h"
#include "file/file_util.h"
#include <pthread.h>

#define _LOG_LEN        512
#define _LOG_LENGTH     2048
#define _LOG_SYSTEM     LOG_LOCAL1
#define _LOG_USER       LOG_LOCAL5

static char _prog_name[_LOG_LEN];

static int  _facility = 0;

static char *getprogname (pid_t pid)
{
    char    cmd[200 + 1];
    FILE    *fp = NULL;
    char    prog_path[_LOG_LEN + 2];
    char    *prog_name = NULL;
    char    *tmp;

    snprintf (cmd, sizeof(cmd), "ps -p %d -o args=", pid);

    if ((fp = popen(cmd, "r")) == NULL)
        return NULL;

    if (fgets(prog_path, sizeof(prog_path), fp) == NULL) {
        prog_name = NULL;
        goto getprogname_exit;
    }

    if (prog_path[strlen(prog_path) - 1] == '\n')
        prog_path[strlen(prog_path) - 1] = '\0';

    tmp = strrchr(prog_path, '/');
    if (tmp == NULL)
        prog_name = strncpy (_prog_name, prog_path, _LOG_LEN);
    else
        prog_name = strncpy (_prog_name, tmp + 1, _LOG_LEN);

getprogname_exit:
    pclose(fp);
    return prog_name;
}

int log_init (int facility)
{
    int facilities[] = {
            LOG_LOCAL0,
            LOG_LOCAL1,
            LOG_LOCAL2,
            LOG_LOCAL3,
            LOG_LOCAL4,
            LOG_LOCAL5,
            LOG_LOCAL6,
            LOG_LOCAL7,
    };

    _facility= facilities[facility];
    if (getprogname (getpid ()) == NULL)
        return -1;
	return 0;
}

#define FPRINTF(stderr, fmt, ...)     fprintf (stderr, fmt, ##__VA_ARGS__)

int log_impl (const char *file, const int line, const char *func, int level, int priority, const char *fmt, ...)
{
    int message_len = 0;
    char message[_LOG_LENGTH];
    char message2[_LOG_LENGTH];
    va_list argptr; 
    pthread_t tid = pthread_self ();
    char *info;

    if (fmt == NULL)
        return 0;

    if (priority != NO_PRIORITY) {
        if (file == NULL || line == 0 || func == NULL)
            message_len += snprintf (message, _LOG_LENGTH, "[Tid: %ld]: ", tid);
        else
            message_len += snprintf (message, _LOG_LENGTH, "[File: %s, Line: %d, Func: %s, Tid: %ld]: ", file, line, func, tid);
    }

    va_start (argptr, fmt); 
    vsnprintf (message2, _LOG_LENGTH, fmt, argptr); 
    va_end (argptr);
    snprintf (message + message_len, _LOG_LENGTH - message_len, "%s", message2);

    int facility1 = LOG_LOCAL6;
    int facility2 = _facility;

    if (level == SYSTEM_LEVEL)
        openlog (_prog_name, LOG_PID|LOG_CONS|LOG_NDELAY, _LOG_SYSTEM);
    else
        openlog (_prog_name, LOG_PID|LOG_CONS|LOG_NDELAY, _facility);

    switch (priority) {
    case NO_PRIORITY:
        syslog (LOG_INFO, "%s", message);
        FPRINTF (stderr, "%s\n", message);
        break;

    case DEBUG_PRIORITY:
#if _DEBUG
        syslog (LOG_DEBUG, "%s %s", "DEBUG", message);
        FPRINTF (stderr, "%s %s\n", "DEBUG", message);
#endif
        break;

    case INFO_PRIORITY:
        syslog (LOG_INFO, "%s %s", "INFO", message);
        FPRINTF (stderr, "%s %s\n", "INFO", message);
        break;

    case NOTICE_PRIORITY:
        syslog (LOG_NOTICE, "%s %s", "NOTICE", message);
        FPRINTF (stderr, "%s %s\n", "NOTICE", message);
        break;

    case WARN_PRIORITY:
        syslog (LOG_WARNING, "%s %s", "WARN", message);
        FPRINTF (stderr, "%s %s\n", "WARN", message);
        break;

    case ERROR_PRIORITY:
        syslog (LOG_ERR, "%s %s", "ERROR", message);
        FPRINTF (stderr, "%s %s\n", "ERROR", message);
        break;

    default:
        return -1;
        break;
    }
    closelog ();

    return 0;
}
