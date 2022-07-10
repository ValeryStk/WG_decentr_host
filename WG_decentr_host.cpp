#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include "json.hpp"
#include "getport.hpp"
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <windows.h>
#include <tchar.h>
#include "StdCapture.h"



namespace fs = std::filesystem;
using json = nlohmann::json;

//pathes
#define STATUS_PATH  "c:\\DecentrWG_config\\WG_status.json"
#define CONFIG_PATH  "c:\\DecentrWG_config\\wg98.conf"
#define WG_HOST_PATH "c:\\DecentrWG_config\\WG_decentr_host.exe"
#define LOG_PATH     "c:\\DecentrWG_config\\WG_log.txt"

//wireguard commands
#define WG_SHOW     "wg show"
#define UNINSTALL_TUNNEL_COMMAND "wireguard /uninstalltunnelservice wg98"
#define WIRE_GUARD_PATH "c:\\DecentrWG"

int setenv(const char* name, const char* value, int overwrite);

bool disconnectActiveConnection();
bool connectToVPN();
bool isWG_installed();
bool isVPN_TunnelInstalled();




int main()
{
   
   
       // set path for wireguard and wg
       setenv("Path", WIRE_GUARD_PATH, 1);
   
        // length of the input and output message
        unsigned long inLength = 0, outLength = 0;
        // in/out message
        std::string inMessage, outMessage;
        std::stringstream outMessage_ss;
        json message;

        // a bit of logging
        std::ofstream logFile;
        logFile.open("MessagingLog.txt", std::ios::app);

        // read the first four bytes (length of input message
        for (int index = 0; index < 4; index++) { unsigned int read_char = getchar(); inLength = inLength | (read_char << index * 8); }

        logFile << "Length: " << inLength << std::endl;

        // read the message form the extension
        for (int index = 0; index < (int)inLength; index++) { inMessage += getchar(); }


        //inMessage = R"({"type":"connect","params": {"ipV4":"10.8.0.3","ipV6":"fd86:ea04:1115:0000:0000:0000:0000:0003","host":"170.187.141.223","port":61409,"hostPublicKey":"BS9Iuy+1LH/Z9uZXUD9FUzb8P9TFnZ4IIfWKxoMMM08=","wgPrivateKey":"UG7PflH9H0OLnrGdprx2WwQ0/YiFJRKe7oRaOivK6l0=","address":"sent1tet7xxem50t6hxfh605ge3r30mau7gl9kd820n","sessionId":55680,"nodeAddress":"sentnode1yfwfsky2usqudsnx7t6xhx4xqsz79zu2va8fws"}})";

        //inMessage = R"({"type":"status"})";

        //inMessage = R"({"type":"disconnect"})";

        //inMessage = R"({"type":"isWgInstalled"})";

        //inMessage = R"({"type":"wgInstall"})";

        logFile << "Received: " << inMessage << std::endl;
        inMessage.erase(std::remove(inMessage.begin(), inMessage.end(), '\\'), inMessage.end());

        try
        {
            logFile << "Is valid json: " << std::boolalpha << json::accept(inMessage) << std::endl;
            message = json::parse(inMessage);
            logFile << "JSON: " << message.dump() << std::endl;

            if (message.find("type") == message.end()) {

                outMessage_ss << "{\"error\":\"invalid json\"}";
            }
            else {

                if (message["type"].get<std::string>() == "connect") {

                    // check that json has required parameters              
                    if (message.find("params") == message.end()) {

                        outMessage_ss << "{\"error\":\"invalid json, no 'params'\"}";
                    }
                    else {

                        uint16_t port = 52487;//GetFreeUDPPort()
                        logFile << "Port: " << port << std::endl;

                        json params = message["params"];
                        // create config file
                        std::ofstream configFile;
                        configFile.open(CONFIG_PATH);
                        configFile << "[Interface]" << std::endl;
                        configFile << "PrivateKey = " << params["wgPrivateKey"].get<std::string>() << std::endl;
                        configFile << "ListenPort = " << port << std::endl;
                        configFile << "Address = " << params["ipV4"].get<std::string>() << "/32, " << params["ipV6"].get<std::string>() << "/128" << std::endl;
                        configFile << "DNS = 10.8.0.1" << std::endl;
                        configFile << "[Peer]" << std::endl;
                        configFile << "PublicKey = " << params["hostPublicKey"].get<std::string>() << std::endl;
                        configFile << "AllowedIPs = 0.0.0.0/0, ::/0" << std::endl;
                        configFile << "Endpoint = " << params["host"].get<std::string>() << ":" << params["port"].get<int>() << std::endl;
                        configFile << "PersistentKeepalive = 15" << std::endl;
                        configFile.close();


                        // first uninstall tunnel service if it is installed
                        disconnectActiveConnection();

                        if (connectToVPN()) {

                            // create status file
                            json status_js;
                            status_js["address"] = params["address"].get<std::string>();
                            status_js["sessionId"] = params["sessionId"].get<int>();
                            status_js["interface"] = "wg98";
                            status_js["nodeAddress"] = params["nodeAddress"].get<std::string>();
                            std::ofstream statusFile;
                            statusFile.open(STATUS_PATH);
                            statusFile << status_js.dump();
                            statusFile.close();
                            outMessage_ss << "{\"result\":true" << ",\"response\":" << status_js.dump() << "}";

                        }
                        else {

                            outMessage_ss << "{\"result\":false}";

                        }
                    }
                }
                else if (message["type"].get<std::string>() == "status")
                {

                    if (isVPN_TunnelInstalled())
                    {

                        if (std::filesystem::exists(STATUS_PATH))
                        {
                            std::ifstream statusFile(STATUS_PATH);
                            std::string line;
                            if (statusFile.is_open())
                            {
                                std::getline(statusFile, line);
                            }
                            outMessage_ss << "{\"result\":true, \"response\":" + line + "}";
                        }
                        else {
                            outMessage_ss << "{\"result\":false}";
                        }

                    }
                    else
                    {
                        outMessage_ss << "{\"result\":false}";
                    }
                }
                else if (message["type"].get<std::string>() == "disconnect")
                {

                    if (disconnectActiveConnection())
                    {
                        std::filesystem::remove(STATUS_PATH);
                        outMessage_ss << "{\"result\":true}";
                    }
                    else {

                        outMessage_ss << "{\"result\":false}";

                    }
                }
                else if (message["type"].get<std::string>() == "isWgInstalled") {

                    if (isWG_installed())
                    {
                        outMessage_ss << "{\"result\":true}";
                    }
                    else
                    {
                        outMessage_ss << "{\"result\":false}";
                    }

                }
                else
                {
                    outMessage_ss << "{\"error\":\"invalid type\"}";
                }
            }

        }
        catch (json::exception& e)
        {
            // output exception information
            logFile << "JSON exception: " << e.what() << ", exception id: " << e.id;
            outMessage_ss << "{\"error\":\"json exception\"}";

        }

        // collect the length of the message
        outLength = outMessage_ss.str().length();

        // send the 4 bytes of length information //
        std::cout.write(reinterpret_cast<const char*>(&outLength), 4);
        // send output message
        std::cout << outMessage_ss.str() << std::flush;

        // a bit of logging
        logFile << "Sent: " << outMessage_ss.str() << std::endl;
        logFile.close();
 

    return EXIT_SUCCESS;
}

bool disconnectActiveConnection() {
    
    int result = -1;
    if(isVPN_TunnelInstalled()) result = system(UNINSTALL_TUNNEL_COMMAND);
    Sleep(100);
    bool isDisconnected = false;
    result == 0 ? isDisconnected = true:isDisconnected = false;
    return isDisconnected;
}

bool connectToVPN() {

    std::string installTunnelCommand = "wireguard /installtunnelservice ";
    installTunnelCommand.append(fs::current_path().string());
    installTunnelCommand.append("\\wg98.conf");
    const char* command = installTunnelCommand.c_str();
    bool isTunnelInstalled = false;
    int result = -1;
    result = std::system(command);
    result == 0 ? isTunnelInstalled = true : isTunnelInstalled = false;
    return isTunnelInstalled;
}

int setenv(const char* name, const char* value, int overwrite)
{
    char* libvar;
    size_t requiredSize;

    getenv_s(&requiredSize, NULL, 0, "Path");
    if (requiredSize == 0)
    {
        exit(1);
    }

    libvar = (char*)malloc(requiredSize * sizeof(char));
    if (!libvar)
    {
        exit(1);
    }

    return _putenv_s(name, value);

}

bool isWG_installed() {

    if (fs::exists(WIRE_GUARD_PATH) && fs::exists(WG_HOST_PATH))
        return true;
    return false;

}

bool isVPN_TunnelInstalled(){
   
    StdCapture stdc;
    stdc.BeginCapture();
    system(WG_SHOW);   
    stdc.EndCapture();
    std::string response = stdc.GetCapture();
    if (!response.empty())return true;
    return false;

}

