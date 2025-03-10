#include <agent_info_persistence.hpp>

#include <logger.hpp>

#include <column.hpp>
#include <persistence.hpp>
#include <persistence_factory.hpp>

using namespace column;

namespace
{
    // database
    const std::string AGENT_INFO_DB_NAME = "agent_info.db";

    // agent_info table
    const std::string AGENT_INFO_TABLE_NAME = "agent_info";
    const std::string AGENT_INFO_NAME_COLUMN_NAME = "name";
    const std::string AGENT_INFO_KEY_COLUMN_NAME = "key";
    const std::string AGENT_INFO_UUID_COLUMN_NAME = "uuid";

    // agent_group table
    const std::string AGENT_GROUP_TABLE_NAME = "agent_group";
    const std::string AGENT_GROUP_ID_COLUMN_NAME = "id";
    const std::string AGENT_GROUP_NAME_COLUMN_NAME = "name";
} // namespace

AgentInfoPersistence::AgentInfoPersistence(const std::string& dbFolderPath, std::unique_ptr<Persistence> persistence)
{
    const auto dbFilePath = dbFolderPath + "/" + AGENT_INFO_DB_NAME;

    try
    {
        if (persistence)
        {
            m_db = std::move(persistence);
        }
        else
        {
            m_db = PersistenceFactory::CreatePersistence(PersistenceFactory::PersistenceType::SQLITE3, dbFilePath);
        }

        if (!m_db->TableExists(AGENT_INFO_TABLE_NAME))
        {
            CreateAgentInfoTable();
        }

        if (!m_db->TableExists(AGENT_GROUP_TABLE_NAME))
        {
            CreateAgentGroupTable();
        }

        if (AgentInfoIsEmpty())
        {
            InsertDefaultAgentInfo();
        }
    }
    catch (const std::exception&)
    {
        throw std::runtime_error(std::string("Cannot open database: " + dbFilePath));
    }
}

AgentInfoPersistence::~AgentInfoPersistence() = default;

bool AgentInfoPersistence::AgentInfoIsEmpty() const
{
    try
    {
        return m_db->GetCount(AGENT_INFO_TABLE_NAME) == 0;
    }
    catch (const std::exception& e)
    {
        LogError("Error fetching: {}.", e.what());
    }

    return false;
}

void AgentInfoPersistence::CreateAgentInfoTable()
{
    try
    {
        const Keys columns = {ColumnKey(AGENT_INFO_NAME_COLUMN_NAME, ColumnType::TEXT, NOT_NULL),
                              ColumnKey(AGENT_INFO_KEY_COLUMN_NAME, ColumnType::TEXT, NOT_NULL),
                              ColumnKey(AGENT_INFO_UUID_COLUMN_NAME, ColumnType::TEXT, NOT_NULL | PRIMARY_KEY)};

        m_db->CreateTable(AGENT_INFO_TABLE_NAME, columns);
    }
    catch (const std::exception& e)
    {
        LogError("Error creating table: {}.", e.what());
        throw;
    }
}

void AgentInfoPersistence::CreateAgentGroupTable()
{
    try
    {
        const Keys columns = {
            ColumnKey(AGENT_GROUP_ID_COLUMN_NAME, ColumnType::INTEGER, NOT_NULL | PRIMARY_KEY | AUTO_INCREMENT),
            ColumnKey(AGENT_GROUP_NAME_COLUMN_NAME, ColumnType::TEXT, NOT_NULL)};

        m_db->CreateTable(AGENT_GROUP_TABLE_NAME, columns);
    }
    catch (const std::exception& e)
    {
        LogError("Error creating table: {}.", e.what());
        throw;
    }
}

void AgentInfoPersistence::InsertDefaultAgentInfo()
{
    try
    {
        const auto count = m_db->GetCount(AGENT_INFO_TABLE_NAME);

        if (count == 0)
        {
            const Row columns = {ColumnValue(AGENT_INFO_NAME_COLUMN_NAME, ColumnType::TEXT, ""),
                                 ColumnValue(AGENT_INFO_KEY_COLUMN_NAME, ColumnType::TEXT, ""),
                                 ColumnValue(AGENT_INFO_UUID_COLUMN_NAME, ColumnType::TEXT, "")};

            m_db->Insert(AGENT_INFO_TABLE_NAME, columns);
        }
    }
    catch (const std::exception& e)
    {
        LogError("Error inserting default agent info: {}.", e.what());
        throw;
    }
}

bool AgentInfoPersistence::SetAgentInfoValue(const std::string& column, const std::string& value)
{
    try
    {
        const Row columns = {ColumnValue(column, ColumnType::TEXT, value)};
        m_db->Update(AGENT_INFO_TABLE_NAME, columns);
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("Error updating {}: {}.", column, e.what());
    }

    return false;
}

std::string AgentInfoPersistence::GetAgentInfoValue(const std::string& column) const
{
    std::string value;

    try
    {
        const Names columns = {ColumnName(column, ColumnType::TEXT)};
        const std::vector<Row> results = m_db->Select(AGENT_INFO_TABLE_NAME, columns);

        if (!results.empty() && !results[0].empty())
        {
            value = results[0][0].Value;
        }
    }
    catch (const std::exception& e)
    {
        LogError("Error fetching {}: {}.", column, e.what());
    }

    return value;
}

std::string AgentInfoPersistence::GetName() const
{
    return GetAgentInfoValue(AGENT_INFO_NAME_COLUMN_NAME);
}

std::string AgentInfoPersistence::GetKey() const
{
    return GetAgentInfoValue(AGENT_INFO_KEY_COLUMN_NAME);
}

std::string AgentInfoPersistence::GetUUID() const
{
    return GetAgentInfoValue(AGENT_INFO_UUID_COLUMN_NAME);
}

std::vector<std::string> AgentInfoPersistence::GetGroups() const
{
    std::vector<std::string> groupList;

    try
    {
        const Names columns = {ColumnName(AGENT_GROUP_NAME_COLUMN_NAME, ColumnType::TEXT)};
        const std::vector<Row> results = m_db->Select(AGENT_GROUP_TABLE_NAME, columns);

        for (const auto& row : results)
        {
            if (!row.empty())
            {
                groupList.push_back(row[0].Value);
            }
        }
    }
    catch (const std::exception& e)
    {
        LogError("Error getting agent group list: {}.", e.what());
    }

    return groupList;
}

bool AgentInfoPersistence::SetName(const std::string& name)
{
    return SetAgentInfoValue(AGENT_INFO_NAME_COLUMN_NAME, name);
}

bool AgentInfoPersistence::SetKey(const std::string& key)
{
    return SetAgentInfoValue(AGENT_INFO_KEY_COLUMN_NAME, key);
}

bool AgentInfoPersistence::SetUUID(const std::string& uuid)
{
    return SetAgentInfoValue(AGENT_INFO_UUID_COLUMN_NAME, uuid);
}

bool AgentInfoPersistence::SetGroups(const std::vector<std::string>& groupList)
{
    TransactionId transaction = 0;

    // Handle the exception separately since it would not be necessary to perform RollBack.
    try
    {
        transaction = m_db->BeginTransaction();
    }
    catch (const std::exception& e)
    {
        LogError("Failed to begin transaction: {}.", e.what());
        return false;
    }

    try
    {
        m_db->Remove(AGENT_GROUP_TABLE_NAME);

        for (const auto& group : groupList)
        {
            const Row columns = {ColumnValue(AGENT_GROUP_NAME_COLUMN_NAME, ColumnType::TEXT, group)};
            m_db->Insert(AGENT_GROUP_TABLE_NAME, columns);
        }

        m_db->CommitTransaction(transaction);
    }
    catch (const std::exception& e)
    {
        LogError("Error inserting group: {}.", e.what());

        try
        {
            m_db->RollbackTransaction(transaction);
        }
        catch (const std::exception& ee)
        {
            LogError("Rollback failed: {}.", ee.what());
        }

        return false;
    }
    return true;
}

bool AgentInfoPersistence::ResetToDefault()
{
    try
    {
        m_db->DropTable(AGENT_INFO_TABLE_NAME);
        m_db->DropTable(AGENT_GROUP_TABLE_NAME);
        CreateAgentInfoTable();
        CreateAgentGroupTable();
        InsertDefaultAgentInfo();
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("Error resetting to default values: {}.", e.what());
    }
    return false;
}
