// UC_Journal.h

#ifndef _UC_JOURNAL_H_
#define _UC_JOURNAL_H_

#include "UH_Define.h"

typedef enum Journal_Level
{
	JL_TOP = 0,
	JL_HIG = 1,
	JL_MID = 2,
	JL_LOW = 3
} JOURNAL_LEVEL;

typedef enum Journal_Type
{
	JT_SCR = 1,
	JT_FIL = 2,
	JT_AND = 3
} JOURNAL_TYPE;

class UC_Journal
{
public:
	// 构造, 析构函数
	UC_Journal()
	{
		m_journal = NULL;
	}	
	~UC_Journal()
	{
		if(m_journal)
			fclose(m_journal);
	}
	
	var_4 init(JOURNAL_TYPE type, JOURNAL_LEVEL level, var_1* file_journal = NULL)
	{
		m_type = type;
		m_level = level;

		if(file_journal == NULL)
			return 0;

		if(access(file_journal, 0))
			m_journal = fopen(file_journal, "w");
		else
			m_journal = fopen(file_journal, "a");
		
		if(m_journal == NULL)
			return -1;

		return 0;
	}
	
	var_4 record(JOURNAL_LEVEL level, const var_1* format, ...)
	{
		if(level > m_level)
			return 0;

		va_list arg_list;

		switch(m_type)
		{
		case 1:
			va_start(arg_list, format);
			vprintf(format, arg_list);
			va_end(arg_list);
			break;

		case 2:
			va_start(arg_list, format);
			vfprintf(m_journal, format, arg_list);
			va_end(arg_list);
			fflush(m_journal);
			break;

		case 3:
			va_start(arg_list, format);
			vprintf(format, arg_list);
			va_end(arg_list);
			va_start(arg_list, format);
			vfprintf(m_journal, format, arg_list);
			va_end(arg_list);
			fflush(m_journal);
			break;
		}

		return 0;
	}
		
	const char* version()
	{
		// v1.000 - 2010.08.18 - 初始版本		
		return "v1.000";
	}

private:
	FILE* m_journal;

	JOURNAL_TYPE  m_type;
	JOURNAL_LEVEL m_level;	
};

#endif // _UC_JOURNAL_H_
