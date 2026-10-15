#pragma once

class KJxScript
{
public:
	// Static Members
	static BOOL		InitFunctions();
	static BOOL		ExecuteScript(const char* szScript);

	// Object Members
	int				LoadBuffer(const char* szCode, int nLength, char* szParam = NULL);
	int				ExecuteCode();
};
