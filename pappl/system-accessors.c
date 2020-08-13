//
// System accessor functions for the Printer Application Framework
//
// Copyright © 2020 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

//
// Include necessary headers...
//

#include "system-private.h"
#ifdef HAVE_LIBJPEG
#  include <jpeglib.h>
#  ifndef JPEG_LIB_VERSION_MAJOR	// Added in JPEGLIB 9
#    define JPEG_LIB_VERSION_MAJOR 8
#    define JPEG_LIB_VERSION_MINOR 0
#  endif // !JPEG_LIB_VERSION_MAJOR
#endif // HAVE_LIBJPEG
#ifdef HAVE_LIBPNG
#  include <png.h>
#endif // HAVE_LIBPNG


//
// Local functions...
//

static bool		add_listeners(pappl_system_t *system, const char *name, int port, int family);
static int		compare_filters(_pappl_mime_filter_t *a, _pappl_mime_filter_t *b);
static _pappl_mime_filter_t *copy_filter(_pappl_mime_filter_t *f);


//
// 'papplSystemAddListeners()' - Add network or domain socket listeners.
//
// The "name" parameter specifies a listener address.  Names starting with a
// slash (/) specify a UNIX domain socket path, otherwise the name is treated
// as a fully-qualified domain name or numeric IPv4 or IPv6 address.  If name
// is `NULL`, the "any" addresses are used.
//
// Listeners cannot be added after @link papplSystemRun@ is called.
//

bool					// O - `true` on success, `false` on failure
papplSystemAddListeners(
    pappl_system_t *system,		// I - System
    const char     *name)		// I - Hostname, domain socket filename, or `NULL`
{
  bool	ret;				// Return value


  if (!system)
  {
    return (false);
  }
  else if (system->is_running)
  {
    papplLog(system, PAPPL_LOGLEVEL_FATAL, "Tried to add listeners while system is running.");
    return (false);
  }

  if (name && *name == '/')
  {
    // Add a domain socket listener...
    ret = add_listeners(system, name, 0, AF_LOCAL);
  }
  else if (name && isdigit(*name & 255))
  {
    // Add IPv4 listener...
    ret = add_listeners(system, name, system->port, AF_INET);
  }
  else if (name && *name == '[')
  {
    // Add IPv6 listener...
    ret = add_listeners(system, name, system->port, AF_INET6);
  }
  else
  {
    // Add named listeners on both IPv4 and IPv6...
    ret = add_listeners(system, name, system->port, AF_INET) || add_listeners(system, name, system->port, AF_INET6);
  }

  return (ret);
}


//
// 'papplSystemAddMIMEFilter()' - Add a file filter to the system.
//
// The "srctype" and "dsttype" arguments specify the source and destination
// MIME media types as constant strings.  A destination MIME media type of
// "image/pwg-raster" specifies a filter that uses the driver's raster
// interface.  Other destination types imply direct submission to the
// output device.
//
// Note: This function may not be called while the system is running.
//

void
papplSystemAddMIMEFilter(
    pappl_system_t         *system,	// I - System
    const char             *srctype,	// I - Source MIME media type (constant) string
    const char             *dsttype,	// I - Destination MIME media type (constant) string
    pappl_mime_filter_cb_t cb,		// I - Filter callback function
    void                   *data)	// I - Filter callback data
{
  _pappl_mime_filter_t	key;		// Search key


  if (!system || system->is_running || !srctype || !dsttype || !cb)
    return;

  if (!system->filters)
    system->filters = cupsArrayNew3((cups_array_func_t)compare_filters, NULL, NULL, 0, (cups_acopy_func_t)copy_filter, (cups_afree_func_t)free);

  key.src    = srctype;
  key.dst    = dsttype;
  key.cb     = cb;
  key.cbdata = data;

  if (!cupsArrayFind(system->filters, &key))
  {
    papplLog(system, PAPPL_LOGLEVEL_DEBUG, "Adding '%s' to '%s' filter.", srctype, dsttype);
    cupsArrayAdd(system->filters, &key);
  }
}


//
// '_papplSystemExportVersions()' - Export the firmware versions to IPP attributes...
//

void
_papplSystemExportVersions(
    pappl_system_t *system,		// I - System
    ipp_t          *ipp,		// I - IPP message
    ipp_tag_t      group_tag,		// I - Group (`IPP_TAG_PRINTER` or `IPP_TAG_SYSTEM`)
    cups_array_t   *ra)			// I - Requested attributes or `NULL` for all
{
  int		i;			// Looping var
  ipp_attribute_t *attr;		// Attribute
  char		name[128];		// Attribute name
  const char	*name_prefix = (group_tag == IPP_TAG_PRINTER) ? "printer" : "system";
  const char	*values[20];		// String values
  char		cups_sversion[32];	// String version of libcups
#ifdef HAVE_LIBJPEG
  char		jpeg_sversion[32];	// String version of libjpeg
#endif // HAVE_LIBJPEG
  unsigned short version[4];		// Version of software components


  // "xxx-firmware-name"
  snprintf(name, sizeof(name), "%s-firmware-name", name_prefix);
  if (!ra || cupsArrayFind(ra, name))
  {
    for (i = 0; i < system->num_versions; i ++)
      values[i] = system->versions[i].name;

    values[i ++] = "PAPPL";

    values[i ++] = "libcups";

#ifdef HAVE_LIBJPEG
    values[i ++] = "libjpeg";
#endif // HAVE_LIBJPEG

#ifdef HAVE_LIBPNG
    values[i ++] = "libpng";
#endif // HAVE_LIBPNG

    ippAddStrings(ipp, group_tag, IPP_TAG_NAME, name, i, NULL, values);
  }

  // "xxx-firmware-patches"
  snprintf(name, sizeof(name), "%s-firmware-patches", name_prefix);
  if (!ra || cupsArrayFind(ra, name))
  {
    for (i = 0; i < system->num_versions; i ++)
      values[i] = system->versions[i].patches;

    values[i ++] = "";			// No patches for PAPPL

    values[i ++] = "";			// No patches for CUPS

#ifdef HAVE_LIBJPEG
    values[i ++] = "";			// No patches for libjpeg
#endif // HAVE_LIBJPEG

#ifdef HAVE_LIBPNG
    values[i ++] = "";			// No patches for libpng
#endif // HAVE_LIBPNG

    ippAddStrings(ipp, group_tag, IPP_TAG_TEXT, name, i, NULL, values);
  }

  // "xxx-firmware-string-version"
  snprintf(name, sizeof(name), "%s-firmware-string-version", name_prefix);
  if (!ra || cupsArrayFind(ra, name))
  {
    for (i = 0; i < system->num_versions; i ++)
      values[i] = system->versions[i].sversion;

    values[i ++] = PAPPL_VERSION;

    snprintf(cups_sversion, sizeof(cups_sversion), "%d.%d.%d", CUPS_VERSION_MAJOR, CUPS_VERSION_MINOR, CUPS_VERSION_PATCH);
    values[i ++] = cups_sversion;

#ifdef HAVE_LIBJPEG
    snprintf(jpeg_sversion, sizeof(jpeg_sversion), "%d.%d", JPEG_LIB_VERSION_MAJOR, JPEG_LIB_VERSION_MINOR);
    values[i ++] = jpeg_sversion;
#endif // HAVE_LIBJPEG

#ifdef HAVE_LIBPNG
    values[i ++] = png_libpng_ver;
#endif // HAVE_LIBPNG

    ippAddStrings(ipp, group_tag, IPP_TAG_TEXT, name, i, NULL, values);
  }

  // "xxx-firmware-version"
  snprintf(name, sizeof(name), "%s-firmware-version", name_prefix);
  if (!ra || cupsArrayFind(ra, name))
  {
    for (i = 0, attr = NULL; i < system->num_versions; i ++)
    {
      if (attr)
	ippSetOctetString(ipp, &attr, ippGetCount(attr), system->versions[i].version, (int)sizeof(system->versions[i].version));
      else
	attr = ippAddOctetString(ipp, group_tag, name, system->versions[i].version, (int)sizeof(system->versions[i].version));
    }

    memset(version, 0, sizeof(version));
    sscanf(PAPPL_VERSION, "%hu.%hu.%hu", version + 0, version + 1, version + 2);
    if (attr)
      ippSetOctetString(ipp, &attr, ippGetCount(attr), version, (int)sizeof(version));
    else
      attr = ippAddOctetString(ipp, group_tag, name, version, (int)sizeof(version));

    version[0] = CUPS_VERSION_MAJOR;
    version[1] = CUPS_VERSION_MINOR;
    version[2] = CUPS_VERSION_PATCH;
    ippSetOctetString(ipp, &attr, ippGetCount(attr), version, (int)sizeof(version));

#ifdef HAVE_LIBJPEG
    version[0] = JPEG_LIB_VERSION_MAJOR;
    version[1] = JPEG_LIB_VERSION_MINOR;
    version[2] = 0;
    ippSetOctetString(ipp, &attr, ippGetCount(attr), version, (int)sizeof(version));
#endif // HAVE_LIBJPEG

#ifdef HAVE_LIBPNG
    memset(version, 0, sizeof(version));
    sscanf(png_libpng_ver, "%hu.%hu.%hu", version + 0, version + 1, version + 2);
    ippSetOctetString(ipp, &attr, ippGetCount(attr), version, (int)sizeof(version));
#endif // HAVE_LIBPNG
  }
}


//
// '_papplSystemFindMIMEFilter()' - Find a filter for the given source and destination formats.
//

_pappl_mime_filter_t *			// O - Filter data
_papplSystemFindMIMEFilter(
    pappl_system_t *system,		// I - System
    const char     *srctype,		// I - Source MIME media type string
    const char     *dsttype)		// I - Destination MIME media type string
{
  _pappl_mime_filter_t	key,		// Search key
			*match;		// Matching filter


  if (!system || !srctype || !dsttype)
    return (NULL);

  pthread_rwlock_rdlock(&system->rwlock);

  key.src = srctype;
  key.dst = dsttype;

  match = (_pappl_mime_filter_t *)cupsArrayFind(system->filters, &key);

  pthread_rwlock_unlock(&system->rwlock);

  return (match);
}


//
// 'papplSystemGetAdminGroup()' - Get the current admin group, if any.
//

char *					// O - Admin group or `NULL` if none
papplSystemGetAdminGroup(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->admin_group)
    {
      strlcpy(buffer, system->admin_group, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetAuthService()' - Get the PAM authorization service, if any.
//

const char *				// O - PAM authorization service or `NULL` if none
papplSystemGetAuthService(
    pappl_system_t *system) 	 	// I - System
{
  return (system ? system->auth_service : NULL);
}


//
// 'papplSystemGetContact()' - Get the "system-contact" value.
//

pappl_contact_t *			// O - Contact
papplSystemGetContact(
    pappl_system_t  *system,		// I - System
    pappl_contact_t *contact)		// O - Contact
{
  if (!system || !contact)
  {
    if (contact)
      memset(contact, 0, sizeof(pappl_contact_t));

    return (contact);
  }

  pthread_rwlock_rdlock(&system->rwlock);

  *contact = system->contact;

  pthread_rwlock_unlock(&system->rwlock);

  return (contact);
}


//
// 'papplSystemGetDefaultPrinterID()' - Get the current "default-printer-id" value.
//

int					// O - "default-printer-id" value
papplSystemGetDefaultPrinterID(
    pappl_system_t *system)		// I - System
{
  return (system ? system->default_printer_id : 0);
}


//
// 'papplSystemGetDefaultPrintGroup()' - Get the default print group, if any.
//

char *					// O - Default print group or `NULL` if none
papplSystemGetDefaultPrintGroup(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->default_print_group)
    {
      strlcpy(buffer, system->default_print_group, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetDNSSDName()' - Get the current DNS-SD service name.
//

char *					// O - Current DNS-SD service name or `NULL` for none
papplSystemGetDNSSDName(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->dns_sd_name)
    {
      strlcpy(buffer, system->dns_sd_name, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetFooterHTML()' - Get the footer HTML for the web interface, if any.
//

const char *				// O - Footer HTML or `NULL` if none
papplSystemGetFooterHTML(
    pappl_system_t *system)		// I - System
{
  return (system ? system->footer_html : NULL);
}


//
// 'papplSystemGetGeoLocation()' - Get the system geo-location string, if any.
//

char *					// O - "geo:" URI or `NULL` for none
papplSystemGetGeoLocation(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->geo_location)
    {
      strlcpy(buffer, system->geo_location, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetHostname()' - Get the system hostname.
//

char *					// O - Hostname
papplSystemGetHostname(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->hostname)
    {
      strlcpy(buffer, system->hostname, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetLocation()' - Get the system location string, if any.
//

char *					// O - Location string or `NULL` for none
papplSystemGetLocation(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->location)
    {
      strlcpy(buffer, system->location, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetLogLevel()' - Get the system log level.
//

pappl_loglevel_t
papplSystemGetLogLevel(
    pappl_system_t *system)     // I - System
{
  return (system ? system->loglevel : PAPPL_LOGLEVEL_UNSPEC);
}

//
// 'papplSystemGetMaxLogSize()' - Get the maximum log file size.
//
// The maximum log size is only used when logging directly to a file.  When the
// limit is reached, the current log file is renamed to "filename.O" and a new
// log file is created.  Set the maximum size to `0` to disable log file
// rotation.
//
// The default maximum log file size is 1MiB or `1048576` bytes.
//

size_t					// O - Maximum log file size or `0` for none
papplSystemGetMaxLogSize(
    pappl_system_t *system)		// I - System
{
  return (system ? system->logmaxsize : 0);
}


//
// 'papplSystemGetName()' - Get the system name string, if any.
//

char *					// O - Name string or `NULL` for none
papplSystemGetName(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->name)
    {
      strlcpy(buffer, system->name, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetNextPrinterID()' - Get the next "printer-id" value.
//

int					// O - Nxt "printer-id" value
papplSystemGetNextPrinterID(
    pappl_system_t *system)		// I - System
{
  return (system ? system->next_printer_id : 0);
}


//
// 'papplSystemGetOptions()' - Get the system options.
//

pappl_soptions_t			// O - Server options
papplSystemGetOptions(
    pappl_system_t *system)		// I - System
{
  return (system ? system->options : PAPPL_SOPTIONS_NONE);
}


//
// 'papplSystemGetOrganization()' - Get the system organization string, if any.
//

char *					// O - Organization string or `NULL` for none
papplSystemGetOrganization(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->organization)
    {
      strlcpy(buffer, system->organization, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetOrganizationalUnit()' - Get the system organizational unit string, if any.
//

char *					// O - Organizational unit string or `NULL` for none
papplSystemGetOrganizationalUnit(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  char	*ret = NULL;			// Return value


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (system->org_unit)
    {
      strlcpy(buffer, system->org_unit, bufsize);
      ret = buffer;
    }
    else
      *buffer = '\0';

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (ret);
}


//
// 'papplSystemGetPassword()' - Get the current password hash.
//
// Note: The access password is only used when the PAM authentication service
// is not set.
//

char *					// O - Password hash
papplSystemGetPassword(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    strlcpy(buffer, system->password_hash, bufsize);

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (buffer);
}


//
// 'papplSystemGetServerHeader()' - Get the Server: header for HTTP responses.
//

const char *				// O - Server: header string or `NULL` for none
papplSystemGetServerHeader(
    pappl_system_t *system)		// I - System
{
  return (system ? system->server_header : NULL);
}


//
// 'papplSystemGetSessionKey()' - Get the current session key.
//
// The session key is used for web interface forms to provide CSRF protection
// and is refreshed periodically.
//

char *					// O - Session key
papplSystemGetSessionKey(
    pappl_system_t *system,		// I - System
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  time_t	curtime = time(NULL);	// Current time


  if (system && buffer && bufsize > 0)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    if ((curtime - system->session_time) > 86400)
    {
      // Update session key with random data...
      snprintf(system->session_key, sizeof(system->session_key), "%08x%08x%08x%08x%08x%08x%08x%08x", _papplGetRand(), _papplGetRand(), _papplGetRand(), _papplGetRand(), _papplGetRand(), _papplGetRand(), _papplGetRand(), _papplGetRand());
      system->session_time = curtime;
    }

    strlcpy(buffer, system->session_key, bufsize);

    pthread_rwlock_unlock(&system->rwlock);
  }
  else if (buffer)
    *buffer = '\0';

  return (buffer);
}


//
// 'papplSystemGetTLSOnly()' - Get the TLS-only state of the system.
//

bool					// O - `true` if the system is only accepting TLS encrypted connections, `false` otherwise
papplSystemGetTLSOnly(
    pappl_system_t *system)		// I - System
{
  return (system ? system->tls_only : false);
}


//
// 'papplSystemGetUUID()' - Get the "system-uuid" value.
//

const char *				// O - "system-uuid" value
papplSystemGetUUID(
    pappl_system_t *system)		// I - System
{
  return (system ? system->uuid : NULL);
}


//
// 'papplSystemGetVersions()' - Get the firmware names and versions.
//

int					// O - Number of firmware versions
papplSystemGetFirmware(
    pappl_system_t  *system,		// I - System
    int             max_versions,	// I - Maximum number of versions to return
    pappl_version_t *versions)		// O - Versions array or `NULL` for don't care
{
  if (versions && max_versions > 0)
    memset(versions, 0, (size_t)max_versions * sizeof(pappl_version_t));

  if (system && versions && system->num_versions > 0)
  {
    pthread_rwlock_rdlock(&system->rwlock);

    if (max_versions > system->num_versions)
      memcpy(versions, system->versions, (size_t)system->num_versions * sizeof(pappl_version_t));
    else
      memcpy(versions, system->versions, (size_t)max_versions * sizeof(pappl_version_t));

    pthread_rwlock_unlock(&system->rwlock);
  }

  return (system ? system->num_versions : 0);
}


//
// 'papplSystemHashPassword()' - Generate a password hash using salt and password strings.
//
// The salt string should be `NULL` to generate a new password hash or the
// value of an existing password hash to verify that a given plaintext password
// string matches the password hash.
//
// Note: Hashes access passwords are only used when the PAM authentication
// service is not set.
//

char *					// O - Hashed password
papplSystemHashPassword(
    pappl_system_t *system,		// I - System
    const char     *salt,		// I - Existing password hash or `NULL` to generate a new hash
    const char     *password,		// I - Plain-text password string
    char           *buffer,		// I - String buffer
    size_t         bufsize)		// I - Size of string buffer
{
  if (system && password && buffer && bufsize > 0)
  {
    char		nonce[100],	// Nonce string
			*ptr,		// Pointer into string
			temp[256];	// Temporary hash
    unsigned char	hash[32];	// SHA2-256 hash

    if (salt && strchr(salt, '~'))
    {
      // Copy existing nonce from the salt string...
      strlcpy(nonce, salt, sizeof(nonce));
      if ((ptr = strchr(nonce, ':')) != NULL)
        *ptr = '\0';
    }
    else
    {
      // Generate a new random nonce...
      snprintf(nonce, sizeof(nonce), "%08x%08x", _papplGetRand(), _papplGetRand());
    }

    snprintf(temp, sizeof(temp), "%s:%s", nonce, password);
    cupsHashData("sha2-256", temp, strlen(temp), hash, sizeof(hash));
    cupsHashString(hash, sizeof(hash), temp, sizeof(temp));

    snprintf(buffer, bufsize, "%s~%s", nonce, temp);
  }
  else if (buffer)
    *buffer = '\0';

  return (buffer);
}


//
// 'papplSystemIsRunning()' - Return whether the system is running.
//

bool					// O - `true` if the system is running, `false` otherwise
papplSystemIsRunning(
    pappl_system_t *system)		// I - System
{
  return (system ? system->is_running : false);
}


//
// 'papplSystemIteratePrinters()' - Iterate all of the printers.
//

void
papplSystemIteratePrinters(
    pappl_system_t     *system,		// I - System
    pappl_printer_cb_t cb,		// I - Callback function
    void               *data)		// I - Callback data
{
  pappl_printer_t	*printer;	// Current printer


  if (!system || !cb)
    return;

  pthread_rwlock_rdlock(&system->rwlock);
  for (printer = (pappl_printer_t *)cupsArrayFirst(system->printers); printer; printer = (pappl_printer_t *)cupsArrayNext(system->printers))
    (cb)(printer, data);
  pthread_rwlock_unlock(&system->rwlock);
}


//
// 'papplSystemSetAdminGroup()' - Set the administrative group.
//

void
papplSystemSetAdminGroup(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - Admin group
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->admin_group);
    system->admin_group = value ? strdup(value) : NULL;

    if (system->admin_group && strcmp(system->admin_group, "none"))
    {
      char		buffer[8192];	// Buffer for strings
      struct group	grpbuf,		// Group buffer
			*grp = NULL;	// Admin group

      if (getgrnam_r(system->admin_group, &grpbuf, buffer, sizeof(buffer), &grp) || !grp)
	papplLog(system, PAPPL_LOGLEVEL_ERROR, "Unable to find admin group '%s'.", system->admin_group);
      else
	system->admin_gid = grp->gr_gid;
    }
    else
      system->admin_gid = (gid_t)-1;

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetContact()' - Set the "system-contact" value.
//

void
papplSystemSetContact(
    pappl_system_t  *system,		// I - System
    pappl_contact_t *contact)		// I - Contact
{
  if (!system || !contact)
    return;

  pthread_rwlock_wrlock(&system->rwlock);

  system->contact = *contact;

  system->config_time = time(NULL);
  system->config_changes ++;

  pthread_rwlock_unlock(&system->rwlock);
}


//
// 'papplSystemSetDefaultPrinterID()' - Set the "default-printer-id" value.
//

void
papplSystemSetDefaultPrinterID(
    pappl_system_t *system,		// I - System
    int            default_printer_id)	// I - "default-printer-id" value
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    system->default_printer_id = default_printer_id;

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetDefaultPrintGroup()' - Set the default print group.
//

void
papplSystemSetDefaultPrintGroup(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - Default print group or `NULL` for none
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->default_print_group);
    system->default_print_group = value ? strdup(value) : NULL;

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetDriverCallback()' - Set the driver callback.
//

void
papplSystemSetDriverCallback(
    pappl_system_t    *system,    // I - System
    pappl_driver_cb_t get_driver_cb)    // I - Get driver callback
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);
    system->get_driver_cb = get_driver_cb;
    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetDNSSDName()' - Set the DNS-SD service name.
//

void
papplSystemSetDNSSDName(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - DNS-SD service name or `NULL` for none
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->dns_sd_name);
    system->dns_sd_name      = value ? strdup(value) : NULL;
    system->dns_sd_collision = false;
    system->dns_sd_serial    = 0;
    system->config_time      = time(NULL);
    system->config_changes ++;

    if (!value)
      _papplSystemUnregisterDNSSDNoLock(system);
    else
      _papplSystemRegisterDNSSDNoLock(system);

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetFooterHTML()' - Set the footer HTML for the web interface.
//
// The footer HTML can only be set prior to calling @link papplSystemRun@.
//

void
papplSystemSetFooterHTML(
    pappl_system_t *system,		// I - System
    const char     *html)		// I - Footer HTML or `NULL` for none
{
  if (system && html && !system->is_running)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->footer_html);
    system->footer_html = strdup(html);

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetGeoLocation()' - Set the geographic location string.
//

void
papplSystemSetGeoLocation(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - "geo:" URI or `NULL` for none
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->geo_location);
    system->geo_location = value ? strdup(value) : NULL;
    system->config_time  = time(NULL);
    system->config_changes ++;

    _papplSystemRegisterDNSSDNoLock(system);

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetHostname()' - Set the system hostname.
//

void
papplSystemSetHostname(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - Hostname or `NULL` for default
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->hostname);

    if (value)
    {
#if !defined(__APPLE__) && !_WIN32
      cups_file_t	*fp;		// Hostname file

      if ((fp = cupsFileOpen("/etc/hostname", "w")) != NULL)
      {
        cupsFilePrintf(fp, "%s\n", value);
        cupsFileClose(fp);
      }
#endif // !__APPLE__ && !_WIN32

      sethostname(value, (int)strlen(value));

      system->hostname = strdup(value);
    }
    else
    {
      char	temp[1024],		// Temporary hostname string
		*ptr;			// Pointer in temporary hostname

#ifdef HAVE_AVAHI
      _pappl_dns_sd_t	master = _papplDNSSDInit(system);
					  // DNS-SD master reference
      const char *avahi_name = avahi_client_get_host_name_fqdn(master);
					  // mDNS hostname

      if (avahi_name)
	strlcpy(temp, avahi_name, sizeof(temp));
      else
#endif /* HAVE_AVAHI */
      httpGetHostname(NULL, temp, sizeof(temp));

      if ((ptr = strstr(temp, ".lan")) != NULL && !ptr[4])
      {
        // Replace hostname.lan with hostname.local
        strlcpy(ptr, ".local", sizeof(temp) - (size_t)(ptr - temp));
      }
      else if ((ptr = strrchr(temp, '.')) == NULL)
      {
        // No domain information, so append .local to hostname...
        ptr = temp + strlen(temp);
        strlcpy(ptr, ".local", sizeof(temp) - (size_t)(ptr - temp));
      }

      system->hostname = strdup(temp);
    }

    system->config_time = time(NULL);
    system->config_changes ++;

    _papplSystemRegisterDNSSDNoLock(system);

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetLocation()' - Set the system location string, if any.
//

void
papplSystemSetLocation(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - Location or `NULL` for none
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->location);
    system->location    = value ? strdup(value) : NULL;
    system->config_time = time(NULL);
    system->config_changes ++;

    _papplSystemRegisterDNSSDNoLock(system);

    pthread_rwlock_unlock(&system->rwlock);
  }
}

//
// 'papplSystemSetLogLevel()' - Set the system log level
//

void
papplSystemSetLogLevel(
    pappl_system_t       *system,		// I - System
    pappl_loglevel_t     loglevel)  // I - Log Level
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    system->loglevel = loglevel;

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}

//
// 'papplSystemSetMaxLogSize()' - Set the maximum log file size in bytes.
//
// The maximum log size is only used when logging directly to a file.  When the
// limit is reached, the current log file is renamed to "filename.O" and a new
// log file is created.  Set the maximum size to `0` to disable log file
// rotation.
//
// The default maximum log file size is 1MiB or `1048576` bytes.
//

void
papplSystemSetMaxLogSize(
    pappl_system_t *system,		// I - System
    size_t         maxsize)		// I - Maximum log size in bytes or `0` for none
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    system->logmaxsize = maxsize;

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetMIMECallback()' - Set the MIME typing callback for the system.
//
// The MIME typing callback extends the built-in MIME typing support for other
// media types that are supported by the application, typically vendor print
// formats.
//
// The callback function receives a buffer containing the initial bytes of the
// document data, the length of the buffer, and the callback data.  It can then
// return `NULL` if the content is not recognized or a constant string
// containing the MIME media type, for example "application/vnd.hp-pcl" for
// PCL print data.
//

void
papplSystemSetMIMECallback(
    pappl_system_t   *system,		// I - System
    pappl_mime_cb_t  cb,		// I - Callback function
    void             *data)		// I - Callback data
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    system->config_time = time(NULL);
    system->mime_cb     = cb;
    system->mime_cbdata = data;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetNextPrinterID()' - Set the next "printer-id" value.
//
// The next printer ID can only be set prior to calling @link papplSystemRun@.
//

void
papplSystemSetNextPrinterID(
    pappl_system_t *system,		// I - System
    int            next_printer_id)	// I - Next "printer-id" value
{
  if (system && !system->is_running)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    system->next_printer_id = next_printer_id;

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetOperationCallback()' - Set the IPP operation callback.
//
// The operation callback can only be set prior to calling @link papplSystemRun@.
//

void
papplSystemSetOperationCallback(
    pappl_system_t    *system,		// I - System
    pappl_ipp_op_cb_t cb,		// I - Callback function
    void              *data)		// I - Callback data
{
  if (system && !system->is_running)
  {
    pthread_rwlock_wrlock(&system->rwlock);
    system->op_cb     = cb;
    system->op_cbdata = data;
    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetOrganization()' - Set the system organization string, if any.
//

void
papplSystemSetOrganization(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - Organization or `NULL` for none
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->organization);
    system->organization = value ? strdup(value) : NULL;

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetOrganizationalUnit()' - Set the system organizational unit string, if any.
//

void
papplSystemSetOrganizationalUnit(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - Organizational unit or `NULL` for none
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->org_unit);
    system->org_unit = value ? strdup(value) : NULL;

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetPassword()' - Set the access password hash string.
//
// The access password hash string is generated using the
// @link papplSystemHashPassword@ function.
//
// Note: The access password is only used when the PAM authentication service
// is not set.
//

void
papplSystemSetPassword(
    pappl_system_t *system,		// I - System
    const char     *hash)		// I - Hash string
{
  if (system && hash)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    strlcpy(system->password_hash, hash, sizeof(system->password_hash));

    system->config_time = time(NULL);
    system->config_changes ++;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetSaveCallback()' - Set the save callback.
//
// 'papplSystemSetPrintDrivers()' - Set the list of print drivers and driver callback.
//

void
papplSystemSetPrintDrivers(
    pappl_system_t      *system,	// I - System
    int                 num_names,	// I - Number of driver names
    const char * const  *names,		// I - Driver names array
    const char * const  *desc,    // I - Driver Description array
    pappl_pdriver_cb_t  cb,		// I - Callback function
    void                *data)		// I - Callback data
{
  if (system)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    system->config_time    = time(NULL);
    system->num_pdrivers   = num_names;
    system->pdrivers       = names;
    system->pdrivers_desc  = desc;
    system->pdriver_cb     = cb;
    system->pdriver_cbdata = data;

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// The save callback can only be set prior to calling @link papplSystemRun@.
//

void
papplSystemSetSaveCallback(
    pappl_system_t  *system,		// I - System
    pappl_save_cb_t cb,			// I - Callback function
    void            *data)		// I - Callback data
{
  if (system && !system->is_running)
  {
    pthread_rwlock_wrlock(&system->rwlock);
    system->save_cb     = cb;
    system->save_cbdata = data;
    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetUUID()' - Set the system UUID.
//
// The UUID can only be set prior to calling @link papplSystemRun@.
//

void
papplSystemSetUUID(
    pappl_system_t *system,		// I - System
    const char     *value)		// I - UUID
{
  if (system && !system->is_running)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    free(system->uuid);

    if (value)
    {
      system->uuid = strdup(value);
    }
    else
    {
      char uuid[64];			// UUID value

      _papplSystemMakeUUID(system, NULL, 0, uuid, sizeof(uuid));
      system->uuid = strdup(uuid);
    }

    _papplSystemRegisterDNSSDNoLock(system);

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'papplSystemSetVersions()' - Set the firmware names and versions.
//
// The firmware information can only be set prior to calling
// @link papplSystemRun@.
//

void
papplSystemSetVersions(
    pappl_system_t  *system,		// I - System
    int             num_versions,	// I - Number of versions
    pappl_version_t *versions)		// I - Firmware versions
{
  if (system && num_versions && versions && !system->is_running)
  {
    pthread_rwlock_wrlock(&system->rwlock);

    if (num_versions > (int)(sizeof(system->versions) / sizeof(system->versions[0])))
      system->num_versions = (int)(sizeof(system->versions) / sizeof(system->versions[0]));
    else
      system->num_versions = num_versions;

    memcpy(system->versions, versions, (size_t)system->num_versions * sizeof(pappl_version_t));

    pthread_rwlock_unlock(&system->rwlock);
  }
}


//
// 'add_listeners()' - Create and add listener sockets to a system.
//

static bool				// O - `true` on success or `false` on failure
add_listeners(
    pappl_system_t *system,		// I - System
    const char     *name,		// I - Host name or `NULL` for any address
    int            port,		// I - Port number
    int            family)		// I - Address family
{
  int			sock;		// Listener socket
  http_addrlist_t	*addrlist,	// Listen addresses
			*addr;		// Current address
  char			service[255];	// Service port


  snprintf(service, sizeof(service), "%d", port);
  if ((addrlist = httpAddrGetList(name, family, service)) == NULL)
  {
    if (name && *name == '/')
      papplLog(system, PAPPL_LOGLEVEL_INFO, "Unable to lookup address(es) for '%s': %s", name, cupsLastErrorString());
    else
      papplLog(system, PAPPL_LOGLEVEL_INFO, "Unable to lookup address(es) for '%s:%d': %s", name ? name : "*", port, cupsLastErrorString());
    return (false);
  }

  if (name && *name == '/')
    papplLog(system, PAPPL_LOGLEVEL_INFO, "Listening for connections on '%s'.", name);
  else
    papplLog(system, PAPPL_LOGLEVEL_INFO, "Listening for connections on '%s:%d'.", name ? name : "*", system->port);

  for (addr = addrlist; addr && system->num_listeners < _PAPPL_MAX_LISTENERS; addr = addr->next)
  {
    if ((sock = httpAddrListen(&(addrlist->addr), port)) < 0)
    {
      char	temp[256];		// String address

      if (name && *name == '/')
	papplLog(system, PAPPL_LOGLEVEL_ERROR, "Unable to create listener socket for '%s': %s", name, cupsLastErrorString());
      else
	papplLog(system, PAPPL_LOGLEVEL_ERROR, "Unable to create listener socket for '%s:%d': %s", httpAddrString(&addr->addr, temp, (int)sizeof(temp)), system->port, cupsLastErrorString());
    }
    else
    {
      system->listeners[system->num_listeners].fd        = sock;
      system->listeners[system->num_listeners ++].events = POLLIN;
    }
  }

  httpAddrFreeList(addrlist);

  return (true);
}


//
// 'compare_filters()' - Compare two filters.
//

static int				// O - Result of comparison
compare_filters(_pappl_mime_filter_t *a,// I - First filter
                _pappl_mime_filter_t *b)// I - Second filter
{
  int	result = strcmp(a->src, b->src);

  if (!result)
    result = strcmp(a->dst, b->dst);

  return (result);
}


//
// 'copy_filter()' - Copy a filter definition.
//

static _pappl_mime_filter_t *		// O - New filter
copy_filter(_pappl_mime_filter_t *f)	// I - Filter definition
{
  _pappl_mime_filter_t	*newf = calloc(1, sizeof(_pappl_mime_filter_t));
					// New filter


  if (newf)
    memcpy(newf, f, sizeof(_pappl_mime_filter_t));

  return (newf);
}
