/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#include <core/trust/store.h>

#include <sqlite3.h>

#include <iostream>
#include <sstream>
#include <mutex>

namespace core
{
namespace trust
{
namespace impl
{
namespace sqlite
{
// A store implementation persisting requests in an sqlite database.
struct Store : public core::trust::Store,
        public std::enable_shared_from_this<Store>
{
    // Describes the table and its schema for storing requests.
    struct Table
    {
        Table() = delete;

        // The name of the table.
        static const std::string& name()
        {
            static const std::string s{"requests"};
            return s;
        }

        // Collects all columns, their names and indices.
        //
        // |-----------------------------------------------------------------------|
        // |  Id : int | ApplicationId : Text | Feature : int | Timestamp : int64 | Answer : int |
        // |-----------------------------------------------------------------------|
        struct Column
        {
            Column() = delete;

            struct Id
            {
                static const std::string& name()
                {
                    static const std::string s{"Id"};
                    return s;
                }

                static const int index = 0;
            };

            struct ApplicationId
            {
                static const std::string& name()
                {
                    static const std::string s{"ApplicationId"};
                    return s;
                }

                static const int index = 1;
            };

            struct Feature
            {
                static const std::string& name()
                {
                    static const std::string s{"Feature"};
                    return s;
                }

                static const int index = 2;
            };

            struct Timestamp
            {
                static const std::string& name()
                {
                    static const std::string s{"Timestamp"};
                    return s;
                }

                static const int index = 3;
            };

            struct Answer
            {
                static const std::string& name()
                {
                    static const std::string s{"Answer"};
                    return s;
                }

                static const int index = 4;
            };
        };
    };

    // An implementation of the query interface for the SQLite-based store.
    struct Query : public core::trust::Store::Query
    {
        // Encodes the parameter-index for the prepared delete statement.
        struct Delete
        {
            static const std::string& statement()
            {
                static const std::string s
                {
                    "DELETE FROM " +
                    Store::Table::name() +
                    " WHERE Id=?;"
                };

                return s;
            }

            struct Parameter
            {
                struct Id { static const int index = 1; };
            };
        };

        // Encodes the parameter-indices for the prepared select statement.
        struct Select
        {
            static const std::string& statement()
            {
                static const std::string select
                {
                    "SELECT * FROM " +
                    Store::Table::name() +
                    " WHERE ApplicationId=IFNULL(?,ApplicationId) AND"
                    " Feature=IFNULL(?,Feature) AND"
                    " (Timestamp BETWEEN IFNULL(?, Timestamp) AND IFNULL(?,Timestamp)) AND"
                    " Answer=IFNULL(?,Answer);"
                };
                return select;
            }

            struct Parameter
            {
                struct ApplicationId { static const int index = 1; };
                struct Feature { static const int index = ApplicationId::index + 1; };
                struct Timestamp
                {
                    struct LowerBound { static const int index = Feature::index + 1; };
                    struct UpperBound { static const int index = LowerBound::index + 1; };
                };
                struct Answer { static const int index = Timestamp::UpperBound::index + 1; };
            };
        };

        // Constructs the query and associates it with its store.
        // That is: As long as a query is alive, the store is kept alive, too.
        Query(const std::shared_ptr<Store>& store) : d{store}
        {
            d.store->throw_if_result_is_error(
                        sqlite3_prepare(
                            d.store->db,
                            Delete::statement().c_str(),
                            Delete::statement().size(),
                            &d.delete_statement,
                            nullptr));

            d.store->throw_if_result_is_error(
                        sqlite3_prepare(
                            d.store->db,
                            Select::statement().c_str(),
                            Select::statement().size(),
                            &d.select_statement,
                            nullptr));

            all();
        }

        Status status() const
        {
            return d.status;
        }

        void for_application_id(const std::string& id)
        {
            d.store->throw_if_result_is_error(
                        sqlite3_bind_text(d.select_statement,
                                          Select::Parameter::ApplicationId::index,
                                          id.c_str(),
                                          id.size(),
                                          nullptr));
        }

        void for_feature(unsigned int feature)
        {
            d.store->throw_if_result_is_error(
                        sqlite3_bind_int(d.select_statement,
                                         Select::Parameter::Feature::index,
                                         feature));
        }

        void for_interval(const Request::Timestamp& begin, const Request::Timestamp& end)
        {
            d.store->throw_if_result_is_error(
                        sqlite3_bind_int64(
                            d.select_statement,
                            Select::Parameter::Timestamp::LowerBound::index,
                            begin.time_since_epoch().count()));
            d.store->throw_if_result_is_error(
                        sqlite3_bind_int64(
                            d.select_statement,
                            Select::Parameter::Timestamp::UpperBound::index,
                            end.time_since_epoch().count()));
        }

        void for_answer(Request::Answer answer)
        {
            d.store->throw_if_result_is_error(
                        sqlite3_bind_int(
                            d.select_statement,
                            Select::Parameter::Answer::index,
                            static_cast<int>(answer)));
        }

        void all()
        {
            d.store->throw_if_result_is_error(sqlite3_bind_null(
                                                  d.select_statement,
                                                  Select::Parameter::ApplicationId::index));
            d.store->throw_if_result_is_error(sqlite3_bind_null(
                                                  d.select_statement,
                                                  Select::Parameter::Feature::index));
            d.store->throw_if_result_is_error(sqlite3_bind_null(
                                                  d.select_statement,
                                                  Select::Parameter::Timestamp::LowerBound::index));
            d.store->throw_if_result_is_error(sqlite3_bind_null(
                                                  d.select_statement,
                                                  Select::Parameter::Timestamp::UpperBound::index));
            d.store->throw_if_result_is_error(sqlite3_bind_null(
                                                  d.select_statement,
                                                  Select::Parameter::Answer::index));
        }

        void execute()
        {
            d.store->throw_if_result_is_error(sqlite3_reset(d.select_statement));
            auto result = sqlite3_step(d.select_statement);

            switch(result)
            {
            case SQLITE_DONE:
                d.status = Status::eor;
                break;
            case SQLITE_ROW:
                d.status = Status::has_more_results;
                break;
            default:
                d.status = Status::error;
                d.error = sqlite3_errstr(result);
                break;
            }
        }

        void next()
        {
            auto result = sqlite3_step(d.select_statement);

            switch(result)
            {
            case SQLITE_DONE:
                d.status = Status::eor;
                break;
            case SQLITE_ROW:
                d.status = Status::has_more_results;
                break;
            default:
                d.status = Status::error;
                d.error = sqlite3_errstr(result);
                break;
            }
        }

        void erase()
        {
            if (Status::eor == d.status)
                throw std::runtime_error("Cannot delete request as query points beyond the result set.");

            auto id = sqlite3_column_int(
                        d.select_statement,
                        Store::Table::Column::Id::index);

            sqlite3_reset(d.delete_statement);
            d.store->throw_if_result_is_error(sqlite3_bind_int(
                                                  d.delete_statement,
                                                  Delete::Parameter::Id::index,
                                                  id));
            d.store->throw_if_result_is_error(sqlite3_step(d.delete_statement));

            next();
        }

        Request current()
        {
            switch(d.status)
            {
            case Status::error: throw Error::QueryIsInErrorState{};
            case Status::eor: throw Error::NoCurrentResult{};
            case Status::armed: throw Error::NoCurrentResult{};
            default:
            {
                trust::Request request;
                auto ptr = reinterpret_cast<const char*>(sqlite3_column_text(
                                                             d.select_statement,
                                                             Store::Table::Column::ApplicationId::index));
                request.from = ptr ? ptr : "";
                request.feature = sqlite3_column_int64(
                            d.select_statement,
                            Store::Table::Column::Feature::index);
                request.when = std::chrono::system_clock::time_point{
                        std::chrono::system_clock::duration
                        {
                            sqlite3_column_int64(
                                d.select_statement,
                                Store::Table::Column::Timestamp::index)
                        }
                };
                request.answer = static_cast<Request::Answer>(
                            sqlite3_column_int64(
                                d.select_statement,
                                Store::Table::Column::Answer::index));
                return request;
            }
            }

            throw std::runtime_error("Oops ... we should never reach here.");
        }

        struct Private
        {
            Private(const std::shared_ptr<Store>& store)
                : store(store)
            {
            }

            ~Private()
            {
                if (delete_statement)
                    sqlite3_finalize(delete_statement);
                if (select_statement)
                    sqlite3_finalize(select_statement);
            }

            std::shared_ptr<Store> store;
            sqlite3_stmt* delete_statement = nullptr;
            sqlite3_stmt* select_statement = nullptr;
            Status status = Status::armed;
            std::string error;
        } d;
    };

    Store();
    ~Store();

    const char* error() const;
    void throw_if_result_is_error(int result);

    void prepare_delete_statement();
    void prepare_drop_statement();
    void prepare_insert_statement();

    void create_table_if_not_exists();

    // From core::trust::Store
    void reset();
    void add(const Request& request);
    std::shared_ptr<core::trust::Store::Query> query();

    std::mutex guard;
    sqlite3* db;
    sqlite3_stmt* delete_statement = nullptr;
    sqlite3_stmt* drop_statement = nullptr;
    sqlite3_stmt* insert_statement = nullptr;
};
}
}
}
}

namespace trust = core::trust;
namespace sqlite = core::trust::impl::sqlite;

sqlite::Store::Store() : db{nullptr}
{
    static const char* db_name = "/tmp/db";
    auto result = sqlite3_open(db_name, &db);

    switch(result)
    {
    case SQLITE_OK: break;
    default: throw trust::Store::Errors::ErrorOpeningStore{sqlite3_errstr(result)};
    }

    sqlite3_extended_result_codes(db, 1);

    create_table_if_not_exists();

    prepare_delete_statement();
    prepare_drop_statement();
    prepare_insert_statement();
}

sqlite::Store::~Store()
{
    sqlite3_finalize(delete_statement);
    sqlite3_finalize(drop_statement);
    sqlite3_finalize(insert_statement);
    sqlite3_close(db);
}

void sqlite::Store::throw_if_result_is_error(int result)
{
    switch(result)
    {
    case SQLITE_OK:
    case SQLITE_DONE:
    case SQLITE_ROW:
        break;
    default:
        throw std::runtime_error(sqlite3_errstr(result) + std::string(": ") + std::string(sqlite3_errmsg(db)));
    }
}

void sqlite::Store::prepare_delete_statement()
{
    if (delete_statement)
    {
        sqlite3_finalize(delete_statement);
        delete_statement = nullptr;
    }

    static const std::string del = "DELETE FROM " + sqlite::Store::Table::name();

    throw_if_result_is_error(sqlite3_prepare(
                                 db,
                                 del.c_str(),
                                 del.size(),
                                 &delete_statement,
                                 nullptr));
}

void sqlite::Store::prepare_drop_statement()
{
    if (drop_statement)
    {
        sqlite3_finalize(drop_statement);
        drop_statement = nullptr;
    }

    static const std::string drop = "DROP TABLE " + sqlite::Store::Table::name();

    throw_if_result_is_error(sqlite3_prepare(
                                 db,
                                 drop.c_str(),
                                 drop.size(),
                                 &drop_statement,
                                 nullptr));
}

void sqlite::Store::prepare_insert_statement()
{
    if (insert_statement)
    {
        sqlite3_finalize(insert_statement);
        insert_statement = nullptr;
    }

    std::stringstream ss;
    ss << "INSERT INTO " << Store::Table::name()
       << " ('ApplicationId','Feature','Timestamp','Answer') VALUES (?,?,?,?);";

    const auto insert = ss.str();
    throw_if_result_is_error(sqlite3_prepare(db,
                                             insert.c_str(),
                                             insert.size(),
                                             &insert_statement,
                                             nullptr));
}

void sqlite::Store::create_table_if_not_exists()
{
    std::stringstream ss;
    ss << "CREATE TABLE IF NOT EXISTS " << sqlite::Store::Table::name() << "("
       << "\"Id\" INTEGER PRIMARY KEY ASC,"
       << "\"ApplicationId\" TEXT NOT NULL,"
       << "\"Feature\" INTEGER,"
       << "\"Timestamp\" BIGINT,"
       << "\"Answer\" INTEGER"
       << ");";
    const std::string create_statement{ss.str()};

    sqlite3_stmt* statement{nullptr};
    throw_if_result_is_error(sqlite3_prepare(
                                 db,
                                 create_statement.c_str(),
                                 create_statement.size(),
                                 &statement,
                                 nullptr));

    throw_if_result_is_error(sqlite3_step(statement));
    sqlite3_finalize(statement);
}

void sqlite::Store::reset()
{
    std::lock_guard<std::mutex> lg(guard);
    try
    {
        throw_if_result_is_error(sqlite3_reset(delete_statement));
        throw_if_result_is_error(sqlite3_step(delete_statement));
    } catch(const std::runtime_error& e)
    {
        throw trust::Store::Errors::ErrorResettingStore{e.what()};
    }
}


void sqlite::Store::add(const trust::Request& request)
{
    std::lock_guard<std::mutex> lg(guard);

    throw_if_result_is_error(sqlite3_reset(insert_statement));

    throw_if_result_is_error(sqlite3_bind_text(
                                 insert_statement,
                                 sqlite::Store::Table::Column::ApplicationId::index,
                                 request.from.c_str(),
                                 request.from.size(),
                                 nullptr));
    throw_if_result_is_error(sqlite3_bind_int(
                                 insert_statement,
                                 sqlite::Store::Table::Column::Feature::index,
                                 request.feature));
    throw_if_result_is_error(sqlite3_bind_int64(
                                 insert_statement,
                                 sqlite::Store::Table::Column::Timestamp::index,
                                 request.when.time_since_epoch().count()));
    throw_if_result_is_error(sqlite3_bind_int(
                                 insert_statement,
                                 sqlite::Store::Table::Column::Answer::index,
                                 static_cast<int>(request.answer)));

    throw_if_result_is_error(sqlite3_step(insert_statement));
}

std::shared_ptr<trust::Store::Query> sqlite::Store::query()
{
    return std::shared_ptr<trust::Store::Query>{new sqlite::Store::Query{shared_from_this()}};
}

std::shared_ptr<core::trust::Store> core::trust::create_default_store()
{
    return std::shared_ptr<trust::Store>{new sqlite::Store()};
}
