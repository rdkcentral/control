/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
#ifndef __CTRLM_DB_ATTR_BLOB_H__
#define __CTRLM_DB_ATTR_BLOB_H__
#include <string>
#include <vector>
#include "ctrlm_db_attr.h"

typedef void* ctrlm_db_stmt_t;

/**
 * @brief ControlMgr Database Object
 * 
 * This class is the base class interface for all supported Database Objects
 */
class ctrlm_db_obj_t {
public:
    /**
     * ControlMgr DB Object Constructor
     * @param key The key in which this attribute is associated with in the DB
     * @param table The table in which this attribute belongs to in the DB
     */
    ctrlm_db_obj_t(const std::string &key, const std::string &table);
    /**
     * ControlMgr DB Object Destructor
     */
    virtual ~ctrlm_db_obj_t();

public:
    /**
     * Implementation for reading an object from the DB
     * @return True if the object was read from the DB, otherwise False
     */
    bool read_db(ctrlm_db_ctx_t ctx);
    /**
     * Implementation for writing an object to the DB
     * @return True if the object was written to the DB, otherwise False
     */
    bool write_db(ctrlm_db_ctx_t ctx);

public:
    /**
     * Interface for class extensions to implement binding the data to the DB statement
     * @param stmt The DB statement to bind the data to
     * @param index The index where the data should be bound to
     * @return The return code for binding the parameter
     */
    virtual int bind_data(ctrlm_db_stmt_t stmt, int index = 2) = 0;
    /**
     * Interface for class extensions to implement extracting the data from the DB statement
     * @param stmt The DB statement to extract the data from
     * @return True if the data was extracted, else False
     */
    virtual bool extract_data(ctrlm_db_stmt_t stmt) = 0;

protected:
    std::string key;
    std::string table;
};

/**
 * @brief ControlMgr Database Blob
 * 
 * This helper class is used within a ctrlm_db_attr_t's read/write db function to 
 * read/write a blob to the DB
 */
class ctrlm_db_blob_t : public ctrlm_db_obj_t {
public:
    /**
     * ControlMgr DB Blob Constructor
     * @param key The key in which this attribute is associated with in the DB
     * @param table The table in which this attribute belongs to in the DB
     */
    ctrlm_db_blob_t(const std::string &key, const std::string &table);
    /**
     * ControlMgr DB Blob Constructor
     * @param key The key in which this attribute is associated with in the DB
     * @param table The table in which this attribute belongs to in the DB
     * @param data Set the blob data to a string's value
     */
    ctrlm_db_blob_t(const std::string &key, const std::string &table, const std::string &data);
    /**
     * ControlMgr DB Blob Constructor
     * @param key The key in which this attribute is associated with in the DB
     * @param table The table in which this attribute belongs to in the DB
     * @param data A parameter to set the blob data to a buffer's value
     * @param length The length of the data
     */
    ctrlm_db_blob_t(const std::string &key, const std::string &table, char *data, size_t length);
    /**
     * ControlMgr DB Blob Destructor
     */
    virtual ~ctrlm_db_blob_t();

public:
    /**
     * Getter function for the blob's data in std::string form
     * @return A std::string containing the blob data
     */
    std::string to_string() const;
    /**
     * Getter function for the blob's data in char buffer
     * @param buffer The buffer in which the data shall be placed
     * @param length The length of the input buffer
     * @return The number of bytes inserted into the buffer, or -1 on error (either buffer is NULL or not big enough)
     */
    int      to_buffer(char *buffer, size_t length);
    /**
     * Setter function for the blob's data from std::string
     * @param str The string that needs to be stored in the blob
     * @return True on successful copy, otherwise False
     */
    bool        from_string(const std::string &str);
    /**
     * Setter function for the blob's data from char buffer
     * @param buffer The buffer that needs to be stored in the blob
     * @param length The length of the buffer
     * @return True on successful copy, otherwise False
     */
    bool        from_buffer(char *buffer, size_t length);

public:
    /**
     * Implementation for binding the data to the DB statement
     * @see ctrlm_db_obj_t::bind_data(ctrlm_db_stmt_t stmt, int index)
     */
    virtual int bind_data(ctrlm_db_stmt_t stmt, int index);
    /**
     * Implementation for extracting the data from the DB statement
     * @see ctrlm_db_obj_t::extract_data(ctrlm_db_stmt_t stmt)
     */
    virtual bool extract_data(ctrlm_db_stmt_t stmt);

private:
    std::vector<char> blob;
};

/**
 * @brief ControlMgr Database UInt64
 * 
 * This helper class is used within a ctrlm_db_attr_t's read/write db function to 
 * read/write a uint64 to the DB
 */
class ctrlm_db_uint64_t : public ctrlm_db_obj_t {
public:
    /**
     * ControlMgr DB UInt64 Constructor
     * @param key The key in which this attribute is associated with in the DB
     * @param table The table in which this attribute belongs to in the DB
     */
    ctrlm_db_uint64_t(const std::string &key, const std::string &table);
    /**
     * ControlMgr DB UInt64 Constructor
     * @param key The key in which this attribute is associated with in the DB
     * @param table The table in which this attribute belongs to in the DB
     * @param data The uint64_t data
     */
    ctrlm_db_uint64_t(const std::string &key, const std::string &table, uint64_t data);
    /**
     * ControlMgr DB UInt64 Destructor
     */
    virtual ~ctrlm_db_uint64_t();

public:
    /**
     * Getter function for the uint64's data.
     * @return The uint64 data
     */
    uint64_t get_uint64() const;
    /**
     * Setter function for the uint64's data.
     * @param data The uint64 data
     */
    void set_uint64(uint64_t data);

public:
    /**
     * Implementation for binding the data to the DB statement
     * @see ctrlm_db_obj_t::bind_data(ctrlm_db_stmt_t stmt, int index)
     */
    virtual int bind_data(ctrlm_db_stmt_t stmt, int index);
    /**
     * Implementation for extracting the data from the DB statement
     * @see ctrlm_db_obj_t::extract_data(ctrlm_db_stmt_t stmt)
     */
    virtual bool extract_data(ctrlm_db_stmt_t stmt);

private:
    uint64_t data;
};

#endif
