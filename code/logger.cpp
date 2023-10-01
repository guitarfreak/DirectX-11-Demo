
extern Logger* theLogger;
void logPrint(char* category, LogPriority priority, char* txt, bool print = false) { theLogger->log(category, priority, txt, print); }
void logPrint(char* category, char* txt, bool print = false) { theLogger->log(category, Log_Note, txt, print); }
void assertLogPrint(bool check, char* category, LogPriority priority, char* txt, bool print = false) { 
	theLogger->assertLog(check, category, priority, txt, print); 
}
void assertLogPrint(bool check, char* category, char* txt, bool print = false) { 
	theLogger->assertLog(check, category, Log_Note, txt, print); 
}