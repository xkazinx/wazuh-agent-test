#pragma once

#include <chrono>
#include <condition_variable>
#include <ctime>
#include <memory>
#include <mutex>
#include <stack>
#include <string>
#include <thread>

#include <commonDefs.h>
#include <dbsync.hpp>
#include <inventoryNormalizer.hpp>
#include <sysInfoInterface.hpp>

#include <command_entry.hpp>
#include <message.hpp>
#include <moduleWrapper.hpp>

#include <boost/asio/awaitable.hpp>


#include <iostream> 

class Benchmark
{
public:
    enum ID
    {   
        PACKAGES,
        PACKAGES_NORMALIZE,
        PACKAGES_REMOVE_EXCLUDED,
        PACKAGES_SYNCTXNROW,
        PACKAGES_GETDELETEDROWS,

        PROCESSES,
        PROCESSES_SYNCTXNROW,
        PROCESSES_GETDELETEDROWS,
        Max,
    };

    static std::string GetIDName(ID id)
    {
        switch(id)
        {
            case PACKAGES:
                return "PACKAGES";
            case PACKAGES_NORMALIZE:
                return "PACKAGES_NORMALIZE";
            case PACKAGES_REMOVE_EXCLUDED:
                return "PACKAGES_REMOVE_EXCLUDED";
            case PACKAGES_SYNCTXNROW:
                return "PACKAGES_SYNCTXNROW";
            case PACKAGES_GETDELETEDROWS:
                return "PACKAGES_GETDELETEDROWS";
            case PROCESSES:
                return "PROCESSES";
            case PROCESSES_SYNCTXNROW:
                return "PROCESSES_SYNCTXNROW";
            case PROCESSES_GETDELETEDROWS:
                return "PROCESSES_GETDELETEDROWS";

            case Max:
                return "";
        }

        return "";
    }

    class Unit
    {        
    public:
        unsigned int m_totalTime = 0;
        unsigned int m_totalCalls = 0;
        ID m_id;

        std::chrono::steady_clock::time_point _start;

        Unit() {}
        
        void Start()
        {
            ++m_totalCalls;
            _start = std::chrono::steady_clock::now();
        }

        void Reset()
        {
            m_totalTime = m_totalCalls = 0;
        }

        void End()
        {
            auto end = std::chrono::steady_clock::now();
            auto dif = std::chrono::duration_cast<std::chrono::milliseconds>(end - _start).count();

            auto tm = std::chrono::duration<unsigned int, std::milli>(dif).count();
            m_totalTime += tm;
        }
    };

    using Array = std::array<Unit, ID::Max>;
    static Array m_list;
    
    static void Start(ID id)
    {
        m_list[id].m_id = id;
        m_list[id].Start();
    }

    static void End(ID id)
    {
        m_list[id].End();
    }

    static void Reset(ID id = ID::Max)
    {
        if (id == ID::Max)
        {
            for(auto & item : m_list)
            {
                item.Reset();
            }
        }
        else 
        {
            m_list[id].Reset();
        }
    }

    static void Print()
    {
        for(auto & item : m_list)
        {
            std::cout << "Benchmark for " << GetIDName(item.m_id) << ": took " << item.m_totalTime << " in " << item.m_totalCalls << " calls" << std::endl;
        }
    }
};  

class Inventory
{
public:
    static Inventory& Instance()
    {
        static Inventory s_instance;
        return s_instance;
    }

    void Start();
    void Setup(std::shared_ptr<const configuration::ConfigurationParser> configurationParser);
    void Stop();
    Co_CommandExecutionResult ExecuteCommand(const std::string command, const nlohmann::json parameters) const;

    const std::string& Name() const
    {
        return m_moduleName;
    };

    void SetPushMessageFunction(const std::function<int(Message)>& pushMessage);

    void Init(const std::shared_ptr<ISysInfo>& spInfo,
              const std::function<void(const std::string&)>& reportDiffFunction,
              const std::string& dbPath,
              const std::string& normalizerConfigPath,
              const std::string& normalizerType);
    virtual void SendDeltaEvent(const std::string& data);

    const std::string& AgentUUID() const
    {
        return m_agentUUID;
    };

    void SetAgentUUID(const std::string& agentUUID)
    {
        m_agentUUID = agentUUID;
    }

private:
    Inventory();
    ~Inventory() = default;
    Inventory(const Inventory&) = delete;
    Inventory& operator=(const Inventory&) = delete;

    void Destroy();

    std::string GetCreateStatement() const;
    nlohmann::json EcsProcessesData(const nlohmann::json& originalData, bool createFields = true);
    nlohmann::json EcsSystemData(const nlohmann::json& originalData, bool createFields = true);
    nlohmann::json EcsHotfixesData(const nlohmann::json& originalData, bool createFields = true);
    nlohmann::json EcsHardwareData(const nlohmann::json& originalData, bool createFields = true);
    nlohmann::json EcsPackageData(const nlohmann::json& originalData, bool createFields = true);
    nlohmann::json EcsPortData(const nlohmann::json& originalData, bool createFields = true);
    nlohmann::json EcsNetworkData(const nlohmann::json& originalData, bool createFields = true);
    nlohmann::json GetNetworkData();
    nlohmann::json GetPortsData();

    void UpdateChanges(const std::string& table, const nlohmann::json& values, const bool isFirstScan);
    void NotifyChange(ReturnTypeCallback result, const nlohmann::json& data, const std::string& table);
    void ProcessEvent(ReturnTypeCallback result, const nlohmann::json& item, const std::string& table);
    nlohmann::json GenerateMessage(ReturnTypeCallback result, const nlohmann::json& item, const std::string& table);
    void
    NotifyEvent(ReturnTypeCallback result, nlohmann::json& msg, const nlohmann::json& item, const std::string& table);

    void TryCatchTask(const std::function<void()>& task) const;
    void ScanHardware();
    void ScanSystem();
    void ScanNetwork();
    void ScanPackages();
    void ScanHotfixes();
    void ScanPorts();
    void ScanProcesses();
    void Scan();
    void SyncLoop();
    void ShowConfig();
    cJSON* Dump() const;
    static void LogErrorInventory(const std::string& log);
    nlohmann::json EcsData(const nlohmann::json& data, const std::string& table, bool createFields = true);
    std::string GetPrimaryKeys(const nlohmann::json& data, const std::string& table);
    std::string CalculateHashId(const nlohmann::json& data, const std::string& table);
    nlohmann::json AddPreviousFields(nlohmann::json& current, const nlohmann::json& previous);
    nlohmann::json
    GenerateStatelessEvent(const std::string& operation, const std::string& type, const nlohmann::json& data);

    void WriteMetadata(const std::string& key, const std::string& value);
    std::string ReadMetadata(const std::string& key);
    void DeleteMetadata(const std::string& key);
    void CleanMetadata();

    void SetJsonField(nlohmann::json& target,
                      const nlohmann::json& source,
                      const std::string& keyPath,
                      const std::string& jsonKey,
                      const std::optional<std::string>& defaultValue,
                      bool createFields);
    void SetJsonFieldArray(nlohmann::json& target,
                           const nlohmann::json& source,
                           const std::string& destPath,
                           const std::string& sourceKey,
                           bool createFields);

    const std::string m_moduleName {"inventory"};
    std::string m_agentUUID {""}; // Agent UUID
    std::shared_ptr<ISysInfo> m_spInfo;
    std::function<void(const std::string&)> m_reportDiffFunction;
    bool m_enabled;              // Main switch
    std::string m_dbFilePath;    // Database path
    std::time_t m_intervalValue; // Scan interval
    bool m_scanOnStart;          // Scan always on start
    bool m_hardware;             // Hardware inventory
    bool m_system;               // System inventory
    bool m_networks;             // Networks inventory
    bool m_packages;             // Installed packages inventory
    bool m_ports;                // Opened ports inventory
    bool m_portsAll;             // Scan only listening ports or all
    bool m_processes;            // Running processes inventory
    bool m_hotfixes;             // Windows hotfixes installed
    bool m_stopping;
    bool m_notify;
    std::unique_ptr<DBSync> m_spDBSync;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    std::unique_ptr<InvNormalizer> m_spNormalizer;
    std::string m_scanTime;
    std::function<int(Message)> m_pushMessage;
    bool m_hardwareFirstScan;  // Hardware first scan flag
    bool m_systemFirstScan;    // System first scan flag
    bool m_networksFirstScan;  // Networks first scan flag
    bool m_packagesFirstScan;  // Installed packages first scan flag
    bool m_portsFirstScan;     // Opened ports first scan flag
    bool m_processesFirstScan; // Running processes first scan flag
    bool m_hotfixesFirstScan;  // Windows hotfixes installed first scan flag
};
