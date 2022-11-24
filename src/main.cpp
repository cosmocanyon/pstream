#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef linux
#endif

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <ranges>

bool start_process(std::stringstream& command) 
{
    STARTUPINFOA startup_info {};
    PROCESS_INFORMATION process_info {};

    bool result = 
        CreateProcessA(0, const_cast<char *>(command.str().c_str()), 
        0, 0, true, CREATE_SUSPENDED/*DETACHED_PROCESS*/, 0, 0, &startup_info, &process_info)
    ;

    if(!result) 
    {
        //something went wrong
        return result;
    } 
   
    std::cout << "dwProcessId: " << process_info.dwProcessId << "\n";
            
    HANDLE hWait = NULL;
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if(!hEvent) 
    {
        std::cerr << "create event error: " << GetLastError() << "\n";
    }
    else if(!RegisterWaitForSingleObject(&hWait, process_info.hProcess, 
        [](void* context, BOOLEAN isTimeOut)
        {
            if(!isTimeOut) 
                SetEvent((HANDLE)context);
        }, 
        hEvent, INFINITE, WT_EXECUTEONLYONCE)) 
    {
        std::cerr << "register wait error: " << GetLastError() << "\n";
    }

    ResumeThread(process_info.hThread);
    CloseHandle(process_info.hThread);

    if(hEvent) 
    {   
        if(hWait)
        {
            WaitForSingleObject(hEvent, INFINITE);
            UnregisterWait(hWait);
        }

        CloseHandle(hEvent);
    }
    /*if(WaitForSingleObject(process_info.hProcess, INFINITE) == WAIT_OBJECT_0)
            std::cout << "dwProcessId: " <<process_info.dwProcessId << "\n";*/
    CloseHandle(process_info.hProcess);

    return result; 
}

int main(int argc, const char* argv[]) 
{
    std::fstream file {"./../data/config.txt"};
    std::vector<std::string> config;
    enum config_index {streamlink, streamlink_args, player, player_args, browser, server, channel, video_id, quality, url_channel, url_chat};

    if(!file.is_open()) {
        return -1;
    } else {
        std::string stmp;

        while (std::getline(file, stmp)) {
            std::string key, value;
            std::istringstream in {stmp};
            std::getline(in, key, '=');
            std::getline(in, value);
            config.push_back(value);
        }

        file.close();
    }
    
    if(argc >= 2)
        config[channel] = argv[1];
    else if(config[channel] == "") {
        std::cout << "enter channel to join: ";
        std::cin >> config[channel];
    }
    std::cout << "connecting to channel: " << config[channel] << "\n";

    do {
        if(config[video_id] == "live" ) {
            config.push_back(config[server] + config[channel]); //url_channel
            break;
        }
        else if(config[video_id] != "") {
            config.push_back(config[server] + config[channel] + "/v/" + config[video_id]); //url_vod
            config[streamlink_args] = config[streamlink_args] + " --player-passthrough hls";
            break;
        }
        else {
            std::cout << "enter VOD ID or live\n";
            std::cin >> config[video_id];
        }
    } while(true);

    config.push_back(config[server]+"popout/"+config[channel]+"/chat"); //url_chat

    for(auto tmp : config) {
        std::cout << tmp << "\n";
    }

#ifdef linux
    std::stringstream command, chat;
    chat << "\"";                             
    chat << "\"" << browser << "\" "; 
    chat << "\"" << popout << "\""; 
    chat << "\"";     
    system(chat.str().c_str());

    command << "\"";                             
    command << "\"" << streamlink << "\" "; 
    command << "\"" << url << "\" "; 
    command << "\"" << quality << "\"";
    command << "\"";  
    system(command.str().c_str());    
#endif

#ifdef _WIN32

    std::stringstream player_exe, browser_exe;

    browser_exe << "\"" << config[browser] << "\" ";
    browser_exe << "\"" << config[url_chat] << "\"";

    player_exe << "\"" << config[streamlink] << "\" ";
    player_exe << config[streamlink_args] << " ";
    player_exe << "\"--player=" << config[player] << " " << config[player_args] << "\" ";
    player_exe << "\"" << config[url_channel] << "\" ";
    player_exe << "\"" << config[quality] << "\"";

    if(!start_process(browser_exe))
        std::cerr << "no browser detected, no problem you can still watch without interaction!\n";

    if(!start_process(player_exe))
        std::cerr << "error playing the stream\n";

#endif

    std::cout << "Closing..." << std::endl;

    return 0;
}