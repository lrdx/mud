#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include "blocking.queue.hpp"
#include "config.hpp"
#include "sysdep.h"

#include <list>
#include <string>
#include <atomic>
#include <thread>

extern FILE *logfile;

void log(const char *format, ...) __attribute__((format(printf, 1, 2)));
void vlog(const char* format, va_list args) __attribute__((format(printf, 1, 0)));
void vlog(const EOutputStream steam, const char *format, va_list args) __attribute__((format(printf, 2, 0)));
void shop_log(const char *format, ...);
void olc_log(const char *format, ...);
void imm_log(const char *format, ...);
void err_log(const char *format, ...);
void ip_log(const char *ip);

// defines for mudlog() //
#define OFF 0
#define CMP 1
#define BRF 2
#define NRM 3
#define LGH 4
#define DEF 5

void mudlog(const char *str, int type, int level, EOutputStream channel, int file);
void mudlog_python(const std::string& str, int type, int level, const EOutputStream channel, int file);

void hexdump(FILE* file, const char *ptr, size_t buflen, const char* title = nullptr);
inline void hexdump(const EOutputStream stream, const char *ptr, size_t buflen, const char* title = nullptr)
{
	hexdump(runtime_config.logs(stream).handle(), ptr, buflen, title);
}

void write_time(FILE *file);

class AbstractLogger
{
public:
	~AbstractLogger() {}

	virtual void operator()(const char* format, ...) __attribute__((format(printf, 2, 3))) = 0;
};

class Logger: public AbstractLogger
{
public:
	class CPrefix
	{
	public:
		CPrefix(Logger& logger, const char* prefix) : m_logger(logger) { logger.push_prefix(prefix); }
		~CPrefix() { m_logger.pop_prefix(); }
		void change_prefix(const char* new_prefix) { m_logger.change_prefix(new_prefix); }

	private:
		Logger& m_logger;
	};

	void operator()(const char* format, ...) override __attribute__((format(printf, 2, 3)));

private:
	void push_prefix(const char* prefix) { m_prefix.push_back(prefix); }
	void change_prefix(const char* new_prefix);
	void pop_prefix();

	std::list<std::string> m_prefix;
};

inline void Logger::change_prefix(const char* new_prefix)
{
	if (!m_prefix.empty())
	{
		m_prefix.back() = new_prefix;
	}
}

inline void Logger::pop_prefix()
{
	if (!m_prefix.empty())
	{
		m_prefix.pop_back();
	}
}

class OutputThread
{
public:
	using message_t = std::pair<std::shared_ptr<char>, std::size_t>;
	using output_queue_t = BlockingQueue<message_t>;

	OutputThread(const std::size_t queue_size);
	~OutputThread();

	void output(const message_t& message) { m_output_queue.push(message); }

private:
	void output_loop();

	output_queue_t m_output_queue;
	std::shared_ptr<std::thread> m_thread;
	std::atomic_bool m_destroying;
};

#endif // __LOGGER_HPP__

/* vim: set ts=4 sw=4 tw=0 noet syntax=cpp :*/
