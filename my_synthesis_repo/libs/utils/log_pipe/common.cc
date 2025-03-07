// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (C) 2023 Gwenhael Goavec-Merou <gwenhael.goavec-merou@trabucayre.com>
 */
#include "common.h"
std::string StatusCodeToString(StatusCode code) {
	switch (code) {
		case StatusCode::SUCCESS:
			return "Success";
		case StatusCode::BAD_REQUEST:
			return "Bad Request";
		case StatusCode::UNAUTHORIZED:
			return "Unauthorized";
		case StatusCode::NOT_FOUND:
			return "Not Found";
		case StatusCode::INTERNAL_SERVER_ERROR:
			return "Internal Server Error";
		default:
			throw std::invalid_argument("Invalid StatusCode value");
	}
}

std::string LevelCodeToString(LevelCode code) {
	switch (code) {
		case LevelCode::INFO_LOG:
			return "Info";
		case LevelCode::WARNING_LOG:
			return "Warning";
		case LevelCode::CRITICAL_LOG:
			return "CRITICAL_LOG";
		case LevelCode::ERROR_LOG:
			return "Error";
		case LevelCode::ALWAYS_LOG:
			return "ALWAYS";
		default:
			throw std::invalid_argument("Invalid LevelCode value");
	}
}

std::string PipeTypeToString(PipeType pipeType) {
	switch (pipeType) {
		case PipeType::LOG:
			return "log";
		case PipeType::DATA:
			return "data";
		case PipeType::CONTROL:
			return "control";
		default:
			throw std::invalid_argument("Invalid PipeType value");
	}
}

std::string LogCategoryToString(LogCategory category) {
    switch (category) {
        case LogCategory::PROJECT:
			return "Project";
        case LogCategory::CORETCL:
			return "CoreTCL";
        case LogCategory::SYNTHESIS:
			return "Synthesis";
        case LogCategory::COMMON:
			return "Common";
        case LogCategory::IP_FLOW:
			return "IP_Flow";
        case LogCategory::DESIGNUTILS:
			return "DesignUtils";
        case LogCategory::DEVICE:
			return "Device";
        case LogCategory::NETLIST:
			return "Netlist";
        default:
			throw std::invalid_argument("Invalid LogCategory value");
    }
}

// 创建Log结构体
LogData LogData::CreateLogStruct(const LevelCode& levelCode,const LogCategory& category_info,const std::string& taskInfo) {
    std::string category = "[" + LogCategoryToString(category_info) + " " + std::to_string(static_cast<int>(category_info)) + "-" + std::to_string(Common::GetNextIndex(LogCategoryToString(category_info))) + "]";
    LogData logData;
    logData.category = category;
    logData.pipe_type = PipeTypeToString(PipeType::LOG);
    logData.level_code = static_cast<int>(levelCode);
    logData.phase = "SYNTHESIS";
    logData.sub_phase = "SYNTHESIS";
    logData.task_info = taskInfo;
    return logData;
}


namespace Common {
    std::string g_father_process_id = "-1";
    std::string g_log_pipe_name     = R"(\\.\pipe\LogPipe_)";     // 日志管道名称
    std::string g_data_pipe_name    = R"(\\.\pipe\DataPipe_)";    // 数据管道名称
    std::string g_control_pipe_name = R"(\\.\pipe\ControlPipe_)"; // 控制管道名称
	std::string g_log_cache = "";
	std::map<std::string, int> indices; // 存储每个类别的消息编号
	// 动态生成消息唯一标号
    int GetNextIndex(const std::string& messagelabel) {
        std::string key = messagelabel;
        return ++indices[key];
    }
    /**
     * 拼接log数据
     * @param pipeName: 命名管道名称
     * @param jsonData: 要发送的 JSON 数据包
     */
	void CreateLogHeader(std::string& log_info) {
		g_log_cache += log_info;
	}

    /**
     * 通过命名管道发送 JSON 数据包
     * @param pipeName: 命名管道名称
     * @param jsonData: 要发送的 JSON 数据包
     */
    void ConnectAndSendJson(PipeType pipeType, nlohmann::json jsonData) {
        if(Common::g_father_process_id != "-1"){
            std::string pipeName = Common::g_log_pipe_name;
            if (pipeType == PipeType::LOG){
				pipeName = Common::g_log_pipe_name + Common::g_father_process_id;
				// 将 JSON 对象序列化为字符串
				jsonData["message_content"] = g_log_cache + jsonData["message_content"].get<std::string>();
			}
            else if (pipeType == PipeType::DATA){
                pipeName = Common::g_data_pipe_name + Common::g_father_process_id;
			}
            else if (pipeType == PipeType::CONTROL)
                pipeName = Common::g_control_pipe_name + Common::g_father_process_id;

			#if defined(_WIN32) || defined(__MINGW32__)
				// 将 std::string 转换为 std::wstring（Windows 使用宽字符）
				std::wstring wpipeName = std::wstring(pipeName.begin(), pipeName.end());
				// 尝试连接到指定名称的命名管道
				HANDLE hPipe = CreateFileW(
					wpipeName.c_str(),              // 命名管道名称
					GENERIC_WRITE | GENERIC_READ,   // 读写权限
					0,                              // 其他进程是否可以访问(0:不共享管道)
					NULL,                           // 默认安全属性
					OPEN_EXISTING,                  // 打开现有管道
					FILE_ATTRIBUTE_NORMAL,          // 同步模式
					NULL                            // 无模板
				);
				// 处理异常
				if (hPipe == INVALID_HANDLE_VALUE) {
					DWORD errorCode = GetLastError();
					if (errorCode == ERROR_FILE_NOT_FOUND) { // 管道不存在
						std::cerr << "Pipe not found: " << pipeName << std::endl;
					} else if (errorCode == ERROR_ACCESS_DENIED) { // 访问被拒绝
						std::cerr << "Access denied to pipe: " << pipeName << std::endl;
					} else {
						std::cerr << "Failed to connect to pipe: " << pipeName << " Error: " << errorCode << std::endl;
					}
					return;
				}

				g_log_cache = "";
				std::string jsonString = jsonData.dump();

				// 发送 JSON 数据到命名管道
				DWORD bytesWritten = 0;
				if (WriteFile(hPipe, jsonString.c_str(), static_cast<DWORD>(jsonString.length()), &bytesWritten, NULL)) {
					std::cout << "JSON message sent to pipe: "<< pipeName << " (" << std::to_string(bytesWritten) << " bytes written)" << std::endl;
				} else {
					DWORD errorCode = GetLastError();
					std::cerr << "Failed to write to pipe: "<< pipeName << " Error: " << std::to_string(errorCode) << std::endl;
				}
				// // 关闭管道句柄
				CloseHandle(hPipe);
			#endif
		}
    }


    nlohmann::json CreateLogJson(LevelCode level_code,const std::string message_content,const std::string task_info) {
        nlohmann::json packet;
        packet["pipe_type"] = PipeTypeToString(PipeType::LOG);            // 日志
        packet["level_code"] = static_cast<int>(level_code);  // 级别
        packet["message_content"] = message_content;                 // 内容
        packet["phase"] = "SYNTHESIS";
        packet["sub_phase"] = "SYNTHESIS";
        packet["category"] = "";
        packet["task_info"] = task_info;
        return packet;
    }


    nlohmann::json CreateDataJson(StatusCode code, const nlohmann::json& data, const std::string& task_info) {
        nlohmann::json packet;
        packet["pipe_type"] = PipeTypeToString(PipeType::DATA);
        packet["status_code"] = static_cast<int>(code);
        packet["data"] = data;
		packet["phase"] = "SYNTHESIS";
        packet["sub_phase"] = "SYNTHESIS";
		packet["task_info"] = task_info;
        return packet;
    }

    nlohmann::json CreateControlJson() {
        return {};
    }
}
