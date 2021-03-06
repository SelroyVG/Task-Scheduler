#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include "sql.h"

using namespace std;

struct config {  
  string mysql_host;
  string mysql_user;
  string mysql_passwd;
  string mysql_db;
  unsigned int mysql_port;
  string mysql_unix_socket;
} config = {"127.0.0.1","db_login","db_password","tasks_scheduler",0,""};

int Daemon(int launchTime, string daemonCommand, int rank, bool restoreTasks, string output_file){
	if (!restoreTasks){
		wSQL("INSERT INTO daemons_log (process_id,command,status,launch_time) VALUES(%u,%q,%q,FROM_UNIXTIME(%u))",rank,daemonCommand.c_str(),"inserted",launchTime);
	}
	time_t now;
	time(&now);
	int timeDelta = launchTime - now;
	if(timeDelta < 0){
		string query = "UPDATE daemons_log SET status=\'denied\' WHERE process_id=" + to_string(rank) + " AND status=\'inserted\'";
		wSQL(query.c_str());
	}
	else if (timeDelta > 0){ 
		sleep(timeDelta - 1);
		
		string query = "UPDATE daemons_log SET status=\'executed\' WHERE process_id=" + to_string(rank) + " AND status=\'inserted\'";
		wSQL(query.c_str());
		ofstream fout_command(output_file, std::ios_base::app);
		if (output_file == "./std_output")
			fout_command << "*** Command: \"" << daemonCommand << "\" Executed at: " << launchTime << " with results below. ***" << endl;
		
		daemonCommand = daemonCommand + " >> " + output_file;
		system(daemonCommand.c_str());
		if (output_file == "./std_output")
			fout_command << "*** End of the output. ***" << endl << endl;
		fout_command.close();
		
	}
	CloseSQL();
	return 0;
}

	
int ParseTime(string mytime){
	/*2016-12-27 11:30:00*/
	string localTime = mytime;
	if (localTime.find("-") == std::string::npos)
		return atoi(localTime.c_str()); // Using UNIX-time
	time_t t = time(NULL);
	tm* timeStruct = localtime(&t);
	int pos;
	string timeSubstring;
	
	pos = localTime.find("-"); // YEAR
	if (pos == std::string::npos)
		return -1;
	timeSubstring = localTime.substr(0, pos);
	timeStruct->tm_year = atoi(timeSubstring.c_str()) - 1900;
	localTime = localTime.substr(pos + 1, localTime.length() - 1);

	pos = localTime.find("-"); // MONTH
	if (pos == std::string::npos)
		return -1;
	timeSubstring = localTime.substr(0, pos);
	timeStruct->tm_mon = atoi(timeSubstring.c_str()) - 1;
	localTime = localTime.substr(pos + 1, localTime.length() - 1);
	
	pos = localTime.find(" "); // DAY
	if (pos == std::string::npos)
		return -1;
	timeSubstring = localTime.substr(0, pos);
	timeStruct->tm_mday = atoi(timeSubstring.c_str());
	localTime = localTime.substr(pos + 1, localTime.length() - 1);
	
	pos = localTime.find(":"); // HOUR
	if (pos == std::string::npos)
		return -1;
	timeSubstring = localTime.substr(0, pos);
	timeStruct->tm_hour = atoi(timeSubstring.c_str());
	localTime = localTime.substr(pos + 1, localTime.length() - 1);
	
	pos = localTime.find(":"); // MINUTE
	if (pos == std::string::npos)
		return -1;
	timeSubstring = localTime.substr(0, pos);
	timeStruct->tm_min = atoi(timeSubstring.c_str());
	localTime = localTime.substr(pos + 1, localTime.length() - 1);
	
	
	
	timeStruct->tm_sec = atoi(localTime.c_str()); // SECOND
	
	return mktime(timeStruct);
}
	
	
int main(int argc, char ** argv) {
	
	string query;
	string daemonCommand;
	string time;
	string output_file = "./std_output";
	bool restoreTasks = false;
	
	int launchTime = 0;
	for (int argCounter = 1; argCounter < argc; argCounter++) 
	{
		string currentAgrument = argv[argCounter];
		
		if ((currentAgrument == "-r") || (currentAgrument == "--restore")){
			restoreTasks = true;
			continue;
		}
		if ((currentAgrument == "-o") || (currentAgrument == "--output")){
			output_file = argv[argCounter + 1];
			continue;
		}		
		
		if ((currentAgrument == "-c") || (currentAgrument == "--command")){
			daemonCommand = argv[argCounter + 1];
			continue;
		}
		if ((currentAgrument == "-t") || (currentAgrument == "--time")){
			time = argv[argCounter + 1];
			launchTime = ParseTime(time);
			if (launchTime == -1){
				cout << "Wrong format of the time string! Right format: YYYY-MM-DD HH:MM:SS" << endl;
				return 1;
			}
			continue;
		}
	}
	
	if (!restoreTasks){
		if(daemonCommand.length() == 0){
			cout << "Error: command not specified!" << endl;
			return 1;
		}
		if(launchTime == 0){
			cout << "Error: launch time not specified!" << endl;
			return 1;
		}
	}
	
	pid_t pid = fork();
	
		
	if (pid == -1)
        return -1;
    else if (pid)
		return 0;
	
	OpenSQL(config.mysql_host.c_str(),config.mysql_user.c_str(),config.mysql_passwd.c_str(), config.mysql_db.c_str(), config.mysql_port,config.mysql_unix_socket.c_str());
	
	umask(0);
    int rank = setsid();
	
	
	if (!restoreTasks){
		Daemon (launchTime, daemonCommand, rank, restoreTasks, output_file);
	}
	else{
		const char * command;
		long long unsigned launch_time;
		for (void * r = rfSQL("SELECT process_id,command,unix_timestamp(launch_time) FROM daemons_log WHERE status='inserted'"); rnSQL(r, &rank, &command, &launch_time);){
			string query = "./taskscheduler -t " + to_string(launch_time) + " -c \"" + command + "\"";
			system(query.c_str());
			query = "UPDATE daemons_log SET status=\'restarted\' WHERE process_id=" + to_string(rank) + " AND status=\'inserted\'";
			wSQL(query.c_str());
		}
	}
	return 0;
}
