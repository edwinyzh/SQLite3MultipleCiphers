/*
** Name:        cipher_config.c
** Purpose:     Configuration of SQLite codecs
** Author:      Ulrich Telle
** Created:     2020-03-02
** Copyright:   (c) 2006-2020 Ulrich Telle
** License:     MIT
*/

#include "cipher_common.h"
#include "cipher_config.h"

/* --- Codec --- */


SQLITE_PRIVATE Codec*
sqlite3mcGetCodec(sqlite3* db, int dbIndex);

SQLITE_PRIVATE void
sqlite3mcConfigTable(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  CodecParameter* codecParams = (CodecParameter*) sqlite3_user_data(context);
  assert(argc == 0);
  sqlite3_result_pointer(context, codecParams, "sqlite3mc_codec_params", 0);
}

SQLITE_PRIVATE CodecParameter*
sqlite3mcGetCodecParams(sqlite3* db)
{
  CodecParameter* codecParams = NULL;
  sqlite3_stmt* pStmt = 0;
  int rc = sqlite3_prepare_v2(db, "SELECT sqlite3mc_config_table();", -1, &pStmt, 0);
  if (rc == SQLITE_OK)
  {
    if (SQLITE_ROW == sqlite3_step(pStmt))
    {
      sqlite3_value* ptrValue = sqlite3_column_value(pStmt, 0);
      codecParams = (CodecParameter*) sqlite3_value_pointer(ptrValue, "sqlite3mc_codec_params");
    }
    sqlite3_finalize(pStmt);
  }
  return codecParams;
}

SQLITE_API int
sqlite3mc_config(sqlite3* db, const char* paramName, int newValue)
{
  int value = -1;
  CodecParameter* codecParams;
  int hasDefaultPrefix = 0;
  int hasMinPrefix = 0;
  int hasMaxPrefix = 0;
  CipherParams* param;

  if (paramName == NULL || (db == NULL && newValue >= 0))
  {
    return value;
  }

  codecParams = (db != NULL) ? sqlite3mcGetCodecParams(db) : globalCodecParameterTable;
  if (codecParams == NULL)
  {
    return value;
  }

  if (sqlite3_strnicmp(paramName, "default:", 8) == 0)
  {
    hasDefaultPrefix = 1;
    paramName += 8;
  }
  if (sqlite3_strnicmp(paramName, "min:", 4) == 0)
  {
    hasMinPrefix = 1;
    paramName += 4;
  }
  if (sqlite3_strnicmp(paramName, "max:", 4) == 0)
  {
    hasMaxPrefix = 1;
    paramName += 4;
  }

  param = codecParams[0].m_params;
  for (; strlen(param->m_name) > 0; ++param)
  {
    if (sqlite3_stricmp(paramName, param->m_name) == 0) break;
  }
  if (strlen(param->m_name) > 0)
  {
    if (db != NULL)
    {
      sqlite3_mutex_enter(db->mutex);
    }
    else
    {
      sqlite3_mutex_enter(sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER));
    }
    value = (hasDefaultPrefix) ? param->m_default : (hasMinPrefix) ? param->m_minValue : (hasMaxPrefix) ? param->m_maxValue : param->m_value;
    if (!hasMinPrefix && !hasMaxPrefix && newValue >= 0 && newValue >= param->m_minValue && newValue <= param->m_maxValue)
    {
      /* Do not allow to change the default value for parameter "hmac_check" */
      if (hasDefaultPrefix && (sqlite3_stricmp(paramName, "hmac_check") != 0))
      {
        param->m_default = newValue;
      }
      param->m_value = newValue;
      value = newValue;
    }
    if (db != NULL)
    {
      sqlite3_mutex_leave(db->mutex);
    }
    else
    {
      sqlite3_mutex_leave(sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER));
    }
  }
  return value;
}

SQLITE_API int
sqlite3mc_config_cipher(sqlite3* db, const char* cipherName, const char* paramName, int newValue)
{
  int value = -1;
  CodecParameter* codecParams;
  CipherParams* cipherParamTable = NULL;
  int j = 0;

  if (cipherName == NULL || paramName == NULL)
  {
    sqlite3_log(SQLITE_WARNING,
                "sqlite3mc_config_cipher: cipher name ('%s*) or parameter ('%s*) missing",
                (cipherName == NULL) ? "" : cipherName, (paramName == NULL) ? "" : paramName);
    return value;
  }
  else if (db == NULL && newValue >= 0)
  {
    sqlite3_log(SQLITE_WARNING,
                "sqlite3mc_config_cipher: global change of parameter '%s' for cipher '%s' not supported",
                paramName, cipherName);
    return value;
  }

  codecParams = (db != NULL) ? sqlite3mcGetCodecParams(db) : globalCodecParameterTable;
  if (codecParams == NULL)
  {
    sqlite3_log(SQLITE_WARNING,
                "sqlite3mc_config_cipher: codec parameter table not found");
    return value;
  }

  for (j = 0; strlen(codecParams[j].m_name) > 0; ++j)
  {
    if (sqlite3_stricmp(cipherName, codecParams[j].m_name) == 0) break;
  }
  if (strlen(codecParams[j].m_name) > 0)
  {
    cipherParamTable = codecParams[j].m_params;
  }

  if (cipherParamTable != NULL)
  {
    int hasDefaultPrefix = 0;
    int hasMinPrefix = 0;
    int hasMaxPrefix = 0;
    CipherParams* param = cipherParamTable;

    if (sqlite3_strnicmp(paramName, "default:", 8) == 0)
    {
      hasDefaultPrefix = 1;
      paramName += 8;
    }
    if (sqlite3_strnicmp(paramName, "min:", 4) == 0)
    {
      hasMinPrefix = 1;
      paramName += 4;
    }
    if (sqlite3_strnicmp(paramName, "max:", 4) == 0)
    {
      hasMaxPrefix = 1;
      paramName += 4;
    }

    /* Special handling for SQLCipher legacy mode */
    if (db != NULL &&
        sqlite3_stricmp(cipherName, "sqlcipher") == 0 &&
        sqlite3_stricmp(paramName, "legacy") == 0)
    {
      if (!hasMinPrefix && !hasMaxPrefix)
      {
        if (newValue > 0 && newValue <= SQLCIPHER_VERSION_MAX)
        {
          sqlite3mcConfigureSQLCipherVersion(db, hasDefaultPrefix, newValue);
        }
        else
        {
          sqlite3_log(SQLITE_WARNING,
                      "sqlite3mc_config_cipher: SQLCipher legacy version %d out of range [%d..%d]",
                      newValue, 1, SQLCIPHER_VERSION_MAX);
        }
      }
    }

    for (; strlen(param->m_name) > 0; ++param)
    {
      if (sqlite3_stricmp(paramName, param->m_name) == 0) break;
    }
    if (strlen(param->m_name) > 0)
    {
      if (db != NULL)
      {
        sqlite3_mutex_enter(db->mutex);
      }
      else
      {
        sqlite3_mutex_enter(sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER));
      }
      value = (hasDefaultPrefix) ? param->m_default : (hasMinPrefix) ? param->m_minValue : (hasMaxPrefix) ? param->m_maxValue : param->m_value;
      if (!hasMinPrefix && !hasMaxPrefix)
      {
        if (newValue >= 0 && newValue >= param->m_minValue && newValue <= param->m_maxValue)
        {
          if (hasDefaultPrefix)
          {
            param->m_default = newValue;
          }
          param->m_value = newValue;
          value = newValue;
        }
        else
        {
          sqlite3_log(SQLITE_WARNING,
                      "sqlite3mc_config_cipher: Value %d for parameter '%s' of cipher '%s' out of range [%d..%d]",
                      newValue, paramName, cipherName, param->m_minValue, param->m_maxValue);
        }
      }
      if (db != NULL)
      {
        sqlite3_mutex_leave(db->mutex);
      }
      else
      {
        sqlite3_mutex_leave(sqlite3_mutex_alloc(SQLITE_MUTEX_STATIC_MASTER));
      }
    }
  }
  return value;
}

SQLITE_API unsigned char*
sqlite3mc_codec_data(sqlite3* db, const char* zDbName, const char* paramName)
{
  unsigned char* result = NULL;
  if (db != NULL && paramName != NULL)
  {
    int iDb = (zDbName != NULL) ? sqlite3FindDbName(db, zDbName) : 0;
    int toRaw = 0;
    if (sqlite3_strnicmp(paramName, "raw:", 4) == 0)
    {
      toRaw = 1;
      paramName += 4;
    }
    if ((sqlite3_stricmp(paramName, "cipher_salt") == 0) && (iDb >= 0))
    {
      /* Check whether database is encrypted */
      Codec* codec = sqlite3mcGetCodec(db, iDb);
      if (codec != NULL && sqlite3mcIsEncrypted(codec) && sqlite3mcHasWriteCipher(codec))
      {
        unsigned char* salt = sqlite3mcGetSaltWriteCipher(codec);
        if (salt != NULL)
        {
          if (!toRaw)
          {
            int j;
            result = sqlite3_malloc(32 + 1);
            for (j = 0; j < 16; ++j)
            {
              result[j * 2] = hexdigits[(salt[j] >> 4) & 0x0F];
              result[j * 2 + 1] = hexdigits[(salt[j]) & 0x0F];
            }
            result[32] = '\0';
          }
          else
          {
            result = sqlite3_malloc(16 + 1);
            memcpy(result, salt, 16);
            result[16] = '\0';
          }
        }
      }
    }
  }
  return result;
}

SQLITE_PRIVATE void
sqlite3mcCodecDataSql(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  const char* nameParam1 = NULL;
  const char* nameParam2 = NULL;

  assert(argc == 1 || argc == 2);
  /* NULL values are not allowed for the first 2 arguments */
  if (SQLITE_NULL == sqlite3_value_type(argv[0]) || (argc > 1 && SQLITE_NULL == sqlite3_value_type(argv[1])))
  {
    sqlite3_result_null(context);
    return;
  }

  /* Determine parameter name */
  nameParam1 = (const char*) sqlite3_value_text(argv[0]);

  /* Determine schema name if given */
  if (argc == 2)
  {
    nameParam2 = (const char*) sqlite3_value_text(argv[1]);
  }

  /* Check for known parameter name(s) */
  if (sqlite3_stricmp(nameParam1, "cipher_salt") == 0)
  {
    /* Determine key salt */
    sqlite3* db = sqlite3_context_db_handle(context);
    const char* salt = (const char*) sqlite3mc_codec_data(db, nameParam2, "cipher_salt");
    if (salt != NULL)
    {
      sqlite3_result_text(context, salt, -1, sqlite3_free);
    }
    else
    {
      sqlite3_result_null(context);
    }
  }
  else
  {
    sqlite3_result_null(context);
  }
}

SQLITE_PRIVATE void
sqlite3mcConfigParams(sqlite3_context* context, int argc, sqlite3_value** argv)
{
  CodecParameter* codecParams;
  const char* nameParam1;
  int hasDefaultPrefix = 0;
  int hasMinPrefix = 0;
  int hasMaxPrefix = 0;
  CipherParams* param1;
  CipherParams* cipherParamTable = NULL;
  int isCommonParam1;
  int isCipherParam1 = 0;

  assert(argc == 1 || argc == 2 || argc == 3);
  /* NULL values are not allowed for the first 2 arguments */
  if (SQLITE_NULL == sqlite3_value_type(argv[0]) || (argc > 1 && SQLITE_NULL == sqlite3_value_type(argv[1])))
  {
    sqlite3_result_null(context);
    return;
  }

  codecParams = (CodecParameter*)sqlite3_user_data(context);

  /* Check first argument whether it is a common parameter */
  /* If the first argument is a common parameter, param1 will point to its parameter table entry */
  nameParam1 = (const char*)sqlite3_value_text(argv[0]);
  if (sqlite3_strnicmp(nameParam1, "default:", 8) == 0)
  {
    hasDefaultPrefix = 1;
    nameParam1 += 8;
  }
  if (sqlite3_strnicmp(nameParam1, "min:", 4) == 0)
  {
    hasMinPrefix = 1;
    nameParam1 += 4;
  }
  if (sqlite3_strnicmp(nameParam1, "max:", 4) == 0)
  {
    hasMaxPrefix = 1;
    nameParam1 += 4;
  }

  param1 = codecParams[0].m_params;
  cipherParamTable = NULL;
  for (; strlen(param1->m_name) > 0; ++param1)
  {
    if (sqlite3_stricmp(nameParam1, param1->m_name) == 0) break;
  }
  isCommonParam1 = strlen(param1->m_name) > 0;

  /* Check first argument whether it is a cipher name, if it wasn't a common parameter */
  /* If the first argument is a cipher name, cipherParamTable will point to the corresponding cipher parameter table */
  if (!isCommonParam1)
  {
    if (!hasDefaultPrefix && !hasMinPrefix && !hasMaxPrefix)
    {
      int j = 0;
      for (j = 0; strlen(codecParams[j].m_name) > 0; ++j)
      {
        if (sqlite3_stricmp(nameParam1, codecParams[j].m_name) == 0) break;
      }
      isCipherParam1 = strlen(codecParams[j].m_name) > 0;
      if (isCipherParam1)
      {
        cipherParamTable = codecParams[j].m_params;
      }
    }
    if (!isCipherParam1)
    {
      /* Prefix not allowed for cipher names or cipher name not found */
      sqlite3_result_null(context);
      return;
    }
  }

  if (argc == 1)
  {
    /* Return value of param1 */
    if (isCommonParam1)
    {
      int value = (hasDefaultPrefix) ? param1->m_default : (hasMinPrefix) ? param1->m_minValue : (hasMaxPrefix) ? param1->m_maxValue : param1->m_value;
      if (sqlite3_stricmp(nameParam1, "cipher") == 0)
      {
        sqlite3_result_text(context, codecDescriptorTable[value - 1]->m_name, -1, SQLITE_STATIC);
      }
      else
      {
        sqlite3_result_int(context, value);
      }
    }
    else if (isCipherParam1)
    {
      /* Return a list of available parameters for the requested cipher */
      int nParams = 0;
      int lenTotal = 0;
      int j;
      for (j = 0; strlen(cipherParamTable[j].m_name) > 0; ++j)
      {
        ++nParams;
        lenTotal += strlen(cipherParamTable[j].m_name);
      }
      if (nParams > 0)
      {
        char* paramList = (char*)sqlite3_malloc(lenTotal + nParams);
        if (paramList != NULL)
        {
          char* p = paramList;
          strcpy(paramList, cipherParamTable[0].m_name);
          for (j = 1; j < nParams; ++j)
          {
            strcat(paramList, ",");
            strcat(paramList, cipherParamTable[j].m_name);
          }
          sqlite3_result_text(context, paramList, -1, sqlite3_free);
        }
        else
        {
          /* Not enough memory to allocate the result */
          sqlite3_result_error_nomem(context);
        }
      }
      else
      {
        /* Cipher has no parameters */
        sqlite3_result_null(context);
      }
    }
  }
  else
  {
    /* 2 or more arguments */
    int arg2Type = sqlite3_value_type(argv[1]);
    if (argc == 2 && isCommonParam1)
    {
      /* Set value of common parameter */
      if (sqlite3_stricmp(nameParam1, "cipher") == 0)
      {
        /* 2nd argument is a cipher name */
        if (arg2Type == SQLITE_TEXT)
        {
          const char* nameCipher = (const char*)sqlite3_value_text(argv[1]);
          int j = 0;
          for (j = 0; strlen(codecDescriptorTable[j]->m_name) > 0; ++j)
          {
            if (sqlite3_stricmp(nameCipher, codecDescriptorTable[j]->m_name) == 0) break;
          }
          if (strlen(codecDescriptorTable[j]->m_name) > 0)
          {
            if (hasDefaultPrefix)
            {
              param1->m_default = j + 1;
            }
            param1->m_value = j + 1;
            sqlite3_result_text(context, codecDescriptorTable[j]->m_name, -1, SQLITE_STATIC);
          }
          else
          {
            /* No match for cipher name found */
            sqlite3_result_null(context);
          }
        }
        else
        {
          /* Invalid parameter type */
          sqlite3_result_null(context);
        }
      }
      else if (arg2Type == SQLITE_INTEGER)
      {
        /* Check that parameter value is within allowed range */
        int value = sqlite3_value_int(argv[1]);
        if (value >= param1->m_minValue && value <= param1->m_maxValue)
        {
          /* Do not allow to change the default value for parameter "hmac_check" */
          if (hasDefaultPrefix && (sqlite3_stricmp(nameParam1, "hmac_check") != 0))
          {
            param1->m_default = value;
          }
          param1->m_value = value;
          sqlite3_result_int(context, value);
        }
        else
        {
          /* Parameter value not within allowed range */
          sqlite3_result_null(context);
        }
      }
      else
      {
        sqlite3_result_null(context);
      }
    }
    else if (isCipherParam1 && arg2Type == SQLITE_TEXT)
    {
      /* get or set cipher parameter */
      const char* nameParam2 = (const char*)sqlite3_value_text(argv[1]);
      CipherParams* param2 = cipherParamTable;
      hasDefaultPrefix = 0;
      if (sqlite3_strnicmp(nameParam2, "default:", 8) == 0)
      {
        hasDefaultPrefix = 1;
        nameParam2 += 8;
      }
      hasMinPrefix = 0;
      if (sqlite3_strnicmp(nameParam2, "min:", 4) == 0)
      {
        hasMinPrefix = 1;
        nameParam2 += 4;
      }
      hasMaxPrefix = 0;
      if (sqlite3_strnicmp(nameParam2, "max:", 4) == 0)
      {
        hasMaxPrefix = 1;
        nameParam2 += 4;
      }
      for (; strlen(param2->m_name) > 0; ++param2)
      {
        if (sqlite3_stricmp(nameParam2, param2->m_name) == 0) break;
      }

      /* Special handling for SQLCipher legacy mode */
      if (argc == 3 &&
        sqlite3_stricmp(nameParam1, "sqlcipher") == 0 &&
        sqlite3_stricmp(nameParam2, "legacy") == 0)
      {
        if (!hasMinPrefix && !hasMaxPrefix && sqlite3_value_type(argv[2]) == SQLITE_INTEGER)
        {
          int legacy = sqlite3_value_int(argv[2]);
          if (legacy > 0 && legacy <= SQLCIPHER_VERSION_MAX)
          {
            sqlite3* db = sqlite3_context_db_handle(context);
            sqlite3mcConfigureSQLCipherVersion(db, hasDefaultPrefix, legacy);
          }
        }
      }

      if (strlen(param2->m_name) > 0)
      {
        if (argc == 2)
        {
          /* Return parameter value */
          int value = (hasDefaultPrefix) ? param2->m_default : (hasMinPrefix) ? param2->m_minValue : (hasMaxPrefix) ? param2->m_maxValue : param2->m_value;
          sqlite3_result_int(context, value);
        }
        else if (!hasMinPrefix && !hasMaxPrefix && sqlite3_value_type(argv[2]) == SQLITE_INTEGER)
        {
          /* Change cipher parameter value */
          int value = sqlite3_value_int(argv[2]);
          if (value >= param2->m_minValue && value <= param2->m_maxValue)
          {
            if (hasDefaultPrefix)
            {
              param2->m_default = value;
            }
            param2->m_value = value;
            sqlite3_result_int(context, value);
          }
          else
          {
            /* Cipher parameter value not within allowed range */
            sqlite3_result_null(context);
          }
        }
        else
        {
          /* Only current value and default value of a parameter can be changed */
          sqlite3_result_null(context);
        }
      }
      else
      {
        /* Cipher parameter not found */
        sqlite3_result_null(context);
      }
    }
    else
    {
      /* Cipher has no parameters */
      sqlite3_result_null(context);
    }
  }
}

SQLITE_PRIVATE int
sqlite3mcConfigureFromUri(sqlite3* db, const char *zDbName, int configDefault)
{
  int rc = SQLITE_OK;

    /* Check URI parameters if database filename is available */
    const char* dbFileName = /*sqlite3_db_filename(db,*/ zDbName /*)*/;
    if (dbFileName != NULL)
    {
      /* Check whether cipher is specified */
      const char* cipherName = sqlite3_uri_parameter(dbFileName, "cipher");
      if (cipherName != NULL)
      {
        int j = 0;
        CipherParams* cipherParams = NULL;

        /* Try to locate the cipher name */
        for (j = 1; strlen(globalCodecParameterTable[j].m_name) > 0; ++j)
        {
          if (sqlite3_stricmp(cipherName, globalCodecParameterTable[j].m_name) == 0) break;
        }

        /* j is the index of the cipher name, if found */
        cipherParams = (strlen(globalCodecParameterTable[j].m_name) > 0) ? globalCodecParameterTable[j].m_params : NULL;
        if (cipherParams != NULL)
        {
          /* Set global parameters (cipher and hmac_check) */
          int hmacCheck = sqlite3_uri_boolean(dbFileName, "hmac_check", 1);
          if (configDefault)
          {
            sqlite3mc_config(db, "default:cipher", j);
          }
          else
          {
            sqlite3mc_config(db, "cipher", j);
          }
          if (!hmacCheck)
          {
            sqlite3mc_config(db, "hmac_check", hmacCheck);
          }

          /* Special handling for SQLCipher */
          if (sqlite3_stricmp(cipherName, "sqlcipher") == 0)
          {
            int legacy = (int) sqlite3_uri_int64(dbFileName, "legacy", 0);
            if (legacy > 0 && legacy <= SQLCIPHER_VERSION_MAX)
            {
              sqlite3mcConfigureSQLCipherVersion(db, configDefault, legacy);
            }
          }

          /* Check all cipher specific parameters */
          for (j = 0; strlen(cipherParams[j].m_name) > 0; ++j)
          {
            int value = (int) sqlite3_uri_int64(dbFileName, cipherParams[j].m_name, -1);
            if (value >= 0)
            {
              /* Configure cipher parameter if it was given in the URI */
              char* param = (configDefault) ? sqlite3_mprintf("default:%s", cipherParams[j].m_name) : cipherParams[j].m_name;
              sqlite3mc_config_cipher(db, cipherName, param, value);
              if (configDefault)
              {
                sqlite3_free(param);
              }
            }
          }
        }
        else
        {
          rc = SQLITE_ERROR;
          sqlite3ErrorWithMsg(db, rc, "unknown cipher '%s'", cipherName);
        }
      }
  }
  return rc;
}

#if 0
SQLITE_API int
wxsqlite3_config(sqlite3* db, const char* paramName, int newValue)
{
  return sqlite3mc_config(db, paramName, newValue);
}

SQLITE_API int
wxsqlite3_config_cipher(sqlite3* db, const char* cipherName, const char* paramName, int newValue)
{
  return sqlite3mc_config_cipher(db, cipherName, paramName, newValue);
}

SQLITE_API unsigned char*
wxsqlite3_codec_data(sqlite3* db, const char* zDbName, const char* paramName)
{
  return sqlite3mc_codec_data(db, zDbName, paramName);
}
#endif