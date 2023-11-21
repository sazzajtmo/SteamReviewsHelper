#ifndef APP_LOG_H
#define APP_LOG_H

class cLogger final
{
private:
	cLogger();
	virtual ~cLogger();

public:
	static	cLogger*		GetInstance();
	static	void			DestroyInstance();

			void			Print( const std::string& format, ... );

private:
	static	cLogger*		s_instance;
};

#define LOG cLogger::GetInstance()->Print

#endif