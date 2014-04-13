#include <log/LogFile.h>
#include <base/noncopyable.h>
#include <base/Types.h>

#ifdef WIN32
#define getcwd _getcwd
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

using namespace thefox;

// not thread safe
class LogFile::File : noncopyable
{
public:
	explicit File(const String& filename)
		: _fp(fopen(filename.data(), "a"))
		, _writtenBytes(0)
	{
		setvbuf(_fp, _buffer, _IOFBF, sizeof(_buffer));
	}

	~File()
	{
		fclose(_fp);
	}

	void append(const char* logline, const size_t len)
	{
		size_t n = write(logline, len);
		size_t remain = len - n;
		while (remain > 0) {
			size_t x = write(logline + n, remain);
			if (x == 0) {
				int err = ferror(_fp);
				if (err) {
					//TODO::������
				}
				break;
			}
			n += x;
			remain = len - n;
		}

		_writtenBytes += len;
	}

	void flush()
	{
		fflush(_fp);
	}

	size_t writtenBytes() const { return _writtenBytes; }

private:
	size_t write(const char* logline, size_t len)
	{
		return ::fwrite(logline, 1, len, _fp);
	}

	FILE* _fp;
	char _buffer[64*1024];
	size_t _writtenBytes;
};


LogFile::LogFile(const String &dir, const String& basename, size_t rollSize, bool threadSafe)
	: _dir(dir)
	, _basename(basename)
	, _rollSize(rollSize)
	, _mutex(threadSafe ? new MutexLock : NULL)
	, _startOfPeriod(0)
	, _lastRoll(0)
{
	makePath(_dir);
	rollFile();
}

LogFile::~LogFile()
{
	
}

void LogFile::append(const char* logline, int len)
{
	if (get_pointer(_mutex)) {
		MutexLockGuard lock(*_mutex);
		append_unlocked(logline, len);
	} else {
		append_unlocked(logline, len);
	}
}

void LogFile::flush()
{
	if (get_pointer(_mutex)) {
		MutexLockGuard lock(*_mutex);
		_file->flush();
	} else {
		_file->flush();
	}
}

void LogFile::append_unlocked(const char* logline, int len)
{
	_file->append(logline, len);

	if (_file->writtenBytes() > _rollSize) {
		rollFile();
	} else {
		time_t now = ::time(NULL);
		time_t _thisPeriod = now / _kRollPerSeconds * _kRollPerSeconds;
		if (_thisPeriod != _startOfPeriod)
			rollFile();
		else 
			_file->flush();
	}
}

void LogFile::rollFile()
{
	time_t now = 0;
	String filename = getLogFileName(_dir, _basename, &now);
	time_t start = now / _kRollPerSeconds * _kRollPerSeconds;

	if (now > _lastRoll) {
		_lastRoll = now;
		_startOfPeriod = start;
		_file.reset(new File(filename));
	}
}

String LogFile::getLogFileName(const String &dir, const String& basename, time_t *now)
{
    String filename;
    
    filename.reserve(dir.size() + basename.size() + 64);
    filename = dir + basename;

    char timebuf[32];
    char pidbuf[32];
  
    *now = time(NULL);
    tm *tm_time = localtime(now);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S", tm_time);
    filename += timebuf;

    snprintf(pidbuf, sizeof pidbuf, ".%d", 
#ifdef WIN32
    ::GetCurrentProcessId()
#else
    getpid()
#endif
    );

    filename += pidbuf;
    filename += ".log";

    return filename;
}

void LogFile::makePath(String &dir)
{   
    char filePath[1000] = {0};
    bool bAbsolutePath = true;

#ifdef WIN32
	if (String::npos == dir.find(':'))
        bAbsolutePath = false;
#else
    if ('/' != dir[0])
        bAbsolutePath = false;
#endif
    
    if (!bAbsolutePath) {
		getcwd(filePath, sizeof(filePath));
        char cSeparator = filePath[strlen(filePath)];
        if (!(cSeparator == '/' || cSeparator == '\\'))
            strcat(filePath, "/");
        
		strncat(filePath, dir.c_str(), sizeof(filePath) - strlen(filePath));
    } else {
		strncpy(filePath, dir.c_str(), sizeof(filePath));
	}
    
    char *curDir = filePath;
    
    while (*curDir != '\0') {
        if (*curDir == '\\' || *curDir == '/') {
            *curDir = '\0';
#ifdef WIN32
            _mkdir(filePath);
#else
	    mkdir(filePath, S_IRWXU);
#endif
            *curDir = '/';
        }
        ++curDir;
    }
#ifdef WIN32
    _mkdir(filePath);
#else
    mkdir(filePath, S_IRWXU);
#endif 
    size_t pathLen = strlen(filePath);
    if ('/' != filePath[pathLen - 1]) {
        strcat(filePath, "/");
        ++pathLen;
    }
    
    dir = filePath;
    return;
}
