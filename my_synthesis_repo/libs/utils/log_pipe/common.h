// SPDX-License-Identifier: Apache-2.0
/*
 * Copyright (C) 2023 Gwenhael Goavec-Merou <gwenhael.goavec-merou@trabucayre.com>
 */

#ifndef SRC_COMMON_HPP_
#define SRC_COMMON_HPP_

#include <string>
#include "libs/nlohmann/json.hpp" // 引入 nlohmann/json
#include "libs/utils/log_pipe/common.h"
#include <iostream>

#if defined(_WIN32) || defined(__MINGW32__)
#include <windows.h>
#endif

// 日志等级
enum class LevelCode {
    ALWAYS_LOG = 0,
    INFO_LOG = 1,
    WARNING_LOG = 2,
    CRITICAL_LOG = 3,
    ERROR_LOG = 4
};

// 日志类别
enum class LogCategory {
    PROJECT = 1,
    CORETCL = 2,
    SYNTHESIS = 8,
    COMMON = 17,
    IP_FLOW = 19,
    DESIGNUTILS = 20,
    DEVICE = 21,
    NETLIST = 22
};

// 管道类型
enum class PipeType {
    LOG = 1,
    DATA = 2,
    CONTROL = 3
};


// 状态码的枚举类
enum class StatusCode {
    SUCCESS = 200,              // 成功
    BAD_REQUEST = 400,          // 客户端请求错误
    UNAUTHORIZED = 401,         // 未授权
    NOT_FOUND = 404,            // 未找到
    INTERNAL_SERVER_ERROR = 500 // 程序内部错误
};

// ---------- CONVERSION FUNCTIONS ----------
std::string LevelCodeToString(LevelCode code);
std::string LogCategoryToString(LogCategory category);
std::string PipeTypeToString(PipeType type);
std::string StatusCodeToString(StatusCode code);

// 定义 LogData 结构体,用来存储进程通信字段
struct LogData {
	std::string pipe_type;
	int level_code;
	std::string message_content;
	std::string phase;
	std::string sub_phase;
	std::string category;
	std::string task_info;

    LogData(){
		pipe_type = PipeTypeToString(PipeType::LOG);
		level_code = static_cast<int>(LevelCode::INFO_LOG);
		message_content = "";
		phase = "SYNTHESIS";
		sub_phase = "SYNTHESIS";
		category = "";
		task_info = "";
	}
	// 创建LogData结构体，用来传输日志信息
    static LogData CreateLogStruct(const LevelCode& levelCode,const LogCategory& category_info,
        const std::string& taskInfo);
};

// 定义全局变量和命名管道的进程通信的逻辑
namespace Common {
    extern std::string g_father_process_id; // 父进程id
    extern std::string g_log_pipe_name;     // 日志管道名称
    extern std::string g_data_pipe_name;    // 数据管道名称
    extern std::string g_control_pipe_name; // 控制管道名称
    extern std::string g_log_cache; // 日志缓存
	void CreateLogHeader(std::string& log_info);
    void ConnectAndSendJson(PipeType pipeType, nlohmann::json jsonData);
	// 动态生成消息唯一标号
    int GetNextIndex(const std::string& messagelabel);
	extern std::map<std::string, int> indices;

    /*!
    * \brief 构造日志类型的 JSON 数据包
    * \param[in] code: 状态码
    * \param[in] message: 日志消息内容
    * \return 构造好的 JSON 数据包
    */
	nlohmann::json CreateLogJson(LevelCode level_code,const std::string message_content,const std::string task_info);

    /*!
    * \brief 构造数据类型的 JSON 数据包
    * \param[in] data: 要传输的数据内容
    * \return 构造好的 JSON 数据包
    */
    nlohmann::json CreateDataJson(StatusCode code, const nlohmann::json& data, const std::string& task_info);

    /*!
    * \brief 构造控制类型的 JSON 数据包
    * \param[in] command: 控制命令
    * \param[in] parameters: 命令的参数（可选）
    * \return 构造好的 JSON 数据包
    */
    nlohmann::json CreateControlJson();
}

#endif  // SRC_COMMON_HPP_
