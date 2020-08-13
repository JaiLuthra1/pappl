//
// Public system header file for the Printer Application Framework
//
// Copyright © 2019-2020 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#ifndef _PAPPL_SYSTEM_H_
#  define _PAPPL_SYSTEM_H_

//
// Include necessary headers...
//

#  include "base.h"
#  include "log.h"


//
// C++ magic...
//

#  ifdef __cplusplus
extern "C" {
#  endif // __cplusplus


//
// Types...
//

enum pappl_soptions_e			// System option bits
{
  PAPPL_SOPTIONS_NONE = 0x0000,			// No options
  PAPPL_SOPTIONS_MULTI_QUEUE = 0x0001,		// Support multiple printers
  PAPPL_SOPTIONS_STANDARD = 0x0002,		// Include the standard web pages
  PAPPL_SOPTIONS_REMOTE_ADMIN = 0x0004,		// Allow remote queue management (vs. localhost only)
  PAPPL_SOPTIONS_NETWORK = 0x0008,		// Include network settings page
  PAPPL_SOPTIONS_SECURITY = 0x0010,		// Include user/password settings page
  PAPPL_SOPTIONS_TLS = 0x0020,			// Include TLS settings page
  PAPPL_SOPTIONS_LOG = 0x0040,			// Include link to log file
  PAPPL_SOPTIONS_DNSSD_HOST = 0x0080,		// Use hostname in DNS-SD service names instead of serial number/UUID
  PAPPL_SOPTIONS_RAW_SOCKET = 0x0100		// Accept jobs via raw sockets
};
typedef unsigned pappl_soptions_t;	// Bitfield for system options

typedef struct pappl_version_s		// Firmware version information
{
  char			name[64],		// "xxx-firmware-name" value
			patches[64],		// "xxx-firmware-patches" value
			sversion[64];		// "xxx-firmware-string-version" value
  unsigned short	version[4];		// "xxx-firmware-version" value
} pappl_version_t;


//
// Callback function types...
//

typedef const char *(*pappl_driver_cb_t)(const char *device_id);
					// Get driver callback
typedef bool (*pappl_pdriver_cb_t)(pappl_system_t *system, const char *driver_name, const char *device_uri, pappl_pdriver_data_t *driver_data, ipp_t **driver_attrs, void *data);
					// Driver callback function
typedef bool (*pappl_mime_filter_cb_t)(pappl_job_t *job, pappl_device_t *device, void *data);
					// Filter callback function
typedef bool (*pappl_ipp_op_cb_t)(pappl_client_t *client, void *data);
					// IPP operation callback function
typedef const char *(*pappl_mime_cb_t)(const unsigned char *header, size_t headersize, void *data);
					// MIME typing callback function
typedef void (*pappl_printer_cb_t)(pappl_printer_t *printer, void *data);
					// Printer iterator callback function
typedef bool (*pappl_resource_cb_t)(pappl_client_t *client, void *data);
					// Dynamic resource callback function
typedef bool (*pappl_save_cb_t)(pappl_system_t *system, void *data);
					// Save callback function


//
// Functions...
//

extern void		papplSystemAddLink(pappl_system_t *system, const char *label, const char *path_or_url, bool secure);
extern bool		papplSystemAddListeners(pappl_system_t *system, const char *name) _PAPPL_PUBLIC;
extern void		papplSystemAddMIMEFilter(pappl_system_t *system, const char *srctype, const char *dsttype, pappl_mime_filter_cb_t cb, void *data) _PAPPL_PUBLIC;
extern void		papplSystemAddResourceCallback(pappl_system_t *system, const char *path, const char *format, pappl_resource_cb_t cb, void *data) _PAPPL_PUBLIC;
extern void		papplSystemAddResourceData(pappl_system_t *system, const char *path, const char *format, const void *data, size_t datalen) _PAPPL_PUBLIC;
extern void		papplSystemAddResourceDirectory(pappl_system_t *system, const char *basepath, const char *directory) _PAPPL_PUBLIC;
extern void		papplSystemAddResourceFile(pappl_system_t *system, const char *path, const char *format, const char *filename) _PAPPL_PUBLIC;
extern void		papplSystemAddResourceString(pappl_system_t *system, const char *path, const char *format, const char *data) _PAPPL_PUBLIC;
extern void		papplSystemAddStringsData(pappl_system_t *system, const char *path, const char *language, const char *data) _PAPPL_PUBLIC;
extern void		papplSystemAddStringsFile(pappl_system_t *system, const char *path, const char *language, const char *filename) _PAPPL_PUBLIC;
extern void		papplSystemCleanJobs(pappl_system_t *system) _PAPPL_PUBLIC;
extern pappl_system_t	*papplSystemCreate(pappl_soptions_t options, const char *name, int port, const char *subtypes, const char *spooldir, const char *logfile, pappl_loglevel_t loglevel, const char *auth_service, bool tls_only) _PAPPL_PUBLIC;
extern void		papplSystemDelete(pappl_system_t *system) _PAPPL_PUBLIC;
extern pappl_printer_t	*papplSystemFindPrinter(pappl_system_t *system, const char *resource, int printer_id, const char *device_uri) _PAPPL_PUBLIC;
extern char		*papplSystemGetAdminGroup(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern const char	*papplSystemGetAuthService(pappl_system_t *system) _PAPPL_PUBLIC;
extern pappl_contact_t	*papplSystemGetContact(pappl_system_t *system, pappl_contact_t *contact) _PAPPL_PUBLIC;
extern int		papplSystemGetDefaultPrinterID(pappl_system_t *system) _PAPPL_PUBLIC;
extern char		*papplSystemGetDefaultPrintGroup(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern char		*papplSystemGetDNSSDName(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern const char	*papplSystemGetFooterHTML(pappl_system_t *system) _PAPPL_PUBLIC;
extern char		*papplSystemGetGeoLocation(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern char		*papplSystemGetHostname(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern char		*papplSystemGetLocation(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern pappl_loglevel_t  papplSystemGetLogLevel(pappl_system_t *system) _PAPPL_PUBLIC;
extern size_t  papplSystemGetMaxLogSize(pappl_system_t *system) _PAPPL_PUBLIC;
extern char		*papplSystemGetName(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern int		papplSystemGetNextPrinterID(pappl_system_t *system) _PAPPL_PUBLIC;
extern pappl_soptions_t	papplSystemGetOptions(pappl_system_t *system) _PAPPL_PUBLIC;
extern char		*papplSystemGetOrganization(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern char		*papplSystemGetOrganizationalUnit(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern char		*papplSystemGetPassword(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern const char	*papplSystemGetServerHeader(pappl_system_t *system) _PAPPL_PUBLIC;
extern char		*papplSystemGetSessionKey(pappl_system_t *system, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern bool		papplSystemGetTLSOnly(pappl_system_t *system) _PAPPL_PUBLIC;
extern const char	*papplSystemGetUUID(pappl_system_t *system) _PAPPL_PUBLIC;
extern int		papplSystemGetVersions(pappl_system_t *system, int max_versions, pappl_version_t *versions) _PAPPL_PUBLIC;
extern char		*papplSystemHashPassword(pappl_system_t *system, const char *salt, const char *password, char *buffer, size_t bufsize) _PAPPL_PUBLIC;
extern bool		papplSystemIsRunning(pappl_system_t *system) _PAPPL_PUBLIC;
extern void		papplSystemIteratePrinters(pappl_system_t *system, pappl_printer_cb_t cb, void *data) _PAPPL_PUBLIC;
extern bool		papplSystemLoadState(pappl_system_t *system, const char *filename) _PAPPL_PUBLIC;
extern void		papplSystemRemoveResource(pappl_system_t *system, const char *path) _PAPPL_PUBLIC;
extern void		papplSystemRun(pappl_system_t *system) _PAPPL_PUBLIC;
extern bool		papplSystemSaveState(pappl_system_t *system, const char *filename) _PAPPL_PUBLIC;

extern void		papplSystemSetAdminGroup(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void		papplSystemSetContact(pappl_system_t *system, pappl_contact_t *contact) _PAPPL_PUBLIC;
extern void		papplSystemSetDefaultPrinterID(pappl_system_t *system, int default_printer_id) _PAPPL_PUBLIC;
extern void		papplSystemSetDefaultPrintGroup(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void   papplSystemSetDriverCallback(pappl_system_t *system, pappl_driver_cb_t get_driver_cb) _PAPPL_PUBLIC;
extern void		papplSystemSetDNSSDName(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void		papplSystemSetFooterHTML(pappl_system_t *system, const char *html) _PAPPL_PUBLIC;
extern void		papplSystemSetGeoLocation(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void		papplSystemSetHostname(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void		papplSystemSetLocation(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void		papplSystemSetLogLevel(pappl_system_t *system, pappl_loglevel_t loglevel) _PAPPL_PUBLIC;
extern void		papplSystemSetMaxLogSize(pappl_system_t *system, size_t maxSize) _PAPPL_PUBLIC;
extern void		papplSystemSetMIMECallback(pappl_system_t *system, pappl_mime_cb_t cb, void *data) _PAPPL_PUBLIC;
extern void		papplSystemSetNextPrinterID(pappl_system_t *system, int next_printer_id) _PAPPL_PUBLIC;
extern void		papplSystemSetOperationCallback(pappl_system_t *system, pappl_ipp_op_cb_t cb, void *data) _PAPPL_PUBLIC;
extern void		papplSystemSetOrganization(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void		papplSystemSetOrganizationalUnit(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void		papplSystemSetPassword(pappl_system_t *system, const char *hash) _PAPPL_PUBLIC;
extern void		papplSystemSetPrintDrivers(pappl_system_t *system, int num_names, const char * const *names, const char * const *desc, pappl_pdriver_cb_t cb, void *data) _PAPPL_PUBLIC;
extern void		papplSystemSetSaveCallback(pappl_system_t *system, pappl_save_cb_t cb, void *data) _PAPPL_PUBLIC;
extern void		papplSystemSetUUID(pappl_system_t *system, const char *value) _PAPPL_PUBLIC;
extern void		papplSystemSetVersions(pappl_system_t *system, int num_versions, pappl_version_t *versions) _PAPPL_PUBLIC;
extern void		papplSystemShutdown(pappl_system_t *system) _PAPPL_PUBLIC;


//
// C++ magic...
//

#  ifdef __cplusplus
}
#  endif // __cplusplus


#endif // !_PAPPL_SYSTEM_H_
