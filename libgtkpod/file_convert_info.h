/*
 * FileConvertInfo.h
 *
 *  Created on: 24-Oct-2009
 *      Author: phantomjinx
 */

#ifndef FILECONVERTINFO_H_
#define FILECONVERTINFO_H_

#ifdef HAVE_CONFIG_H
#   include <config.h>
#endif

#include "itdb.h"

extern const gchar *FILE_CONVERT_CACHEDIR;
extern const gchar *FILE_CONVERT_MAXDIRSIZE;
extern const gchar *FILE_CONVERT_TEMPLATE;
extern const gchar *FILE_CONVERT_MAX_THREADS_NUM;
extern const gchar *FILE_CONVERT_DISPLAY_LOG;
extern const gchar *FILE_CONVERT_BACKGROUND_TRANSFER;

typedef enum
{
    FILE_CONVERT_INACTIVE = 0,
    FILE_CONVERT_REQUIRED,
    FILE_CONVERT_SCHEDULED,
    FILE_CONVERT_FAILED,
    FILE_CONVERT_REQUIRED_FAILED,
    FILE_CONVERT_KILLED,
    FILE_CONVERT_CONVERTED
} FileConvertStatus;

typedef enum
{
    FILE_TRANSFER_ERROR = -1,
    FILE_TRANSFER_IDLE = 0,
    FILE_TRANSFER_ACTIVE,
    FILE_TRANSFER_DISK_FULL
} FileTransferStatus;

#endif /* FILECONVERTINFO_H_ */
