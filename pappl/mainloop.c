//
// Command line utilities for the Printer Application Framework
//
// Copyright © 2020 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

//
// Include necessary headers
//

#  include "pappl-private.h"
#  include <spawn.h>
#  include <libgen.h>


//
// Local functions
//

static void	usage(const char *base_name);


//
// 'papplMainloop()' - Run a standard main loop for printer applications.
//

int					// O - Exit status
papplMainloop(
    int                       argc,		// I - Number of command line arguments
    char                      *argv[],		// I - Command line arguments
    const char                *version,	// I - Version number
    pappl_ml_usage_cb_t       usage_cb,	// I - Usage callback
    const char                *subcmd_name,	// I - Sub-command name
    pappl_ml_subcmd_cb_t      subcmd_cb,	// I - Sub-command callback
    pappl_ml_system_cb_t      system_cb,	// I - System callback
    pappl_ml_driver_cb_t      get_driver_cb,   // I - Get driver callback
    void                      *data)		// I - Context
{
  const char	*base_name;		// Base Name
  int		i, j;			// Looping vars
  const char	*opt;			// Option character
  const char	*subcommand = NULL;	// Sub-command
  int		num_files = 0;		// File count
  char		*files[1000];		// Files array
  int		num_options = 0;	// Number of options
  cups_option_t	*options = NULL;	// Options
  static const char * const subcommands[] =
  {					// List of standard sub-commands
    "add",
    "auto-add",
    "cancel",
    "default",
    "delete",
    "devices",
    "drivers",
    "jobs",
    "modify",
    "options",
    "printers",
    "server",
    "shutdown",
    "status",
    "submit"
  };


  // Range check input...
  if (argc < 1 || !argv || !version || !system_cb)
    return (1);

  // Save the path to the printer application and get the base name.
  _papplMainloopPath = argv[0];
  base_name          = basename(argv[0]);

  // Parse the command-line...
  for (i = 1; i < argc; i ++)
  {
    if (!strcmp(argv[i], "--help"))
    {
      if (usage_cb)
        (*usage_cb)(data);
      else
        usage(base_name);

      return (0);
    }
    else if (!strcmp(argv[i], "--version"))
    {
      puts(version);
      return (0);
    }
    else if (!strcmp(argv[i], "--"))
    {
      // Filename follows
      i ++;

      if (i >= argc)
      {
        fprintf(stderr, "%s: Missing filename after '--'.\n", base_name);
        return (1);
      }
      else if (num_files >= (int)(sizeof(files) / sizeof(files[0])))
      {
        fprintf(stderr, "%s: Too many files.\n", base_name);
        return (1);
      }

      files[num_files ++] = argv[i];
    }
    else if (!strncmp(argv[i], "--", 2))
    {
      fprintf(stderr, "%s: Unknown option '%s'.\n", base_name, argv[i]);
      return (1);
    }
    else if (!strcmp(argv[i], "-") || argv[i][0] != '-')
    {
      // See if this is a standard sub-command...
      for (j = 0; j < (int)(sizeof(subcommands) / sizeof(subcommands[0])); j ++)
      {
        if (!strcmp(argv[i], subcommands[j]))
        {
          if (subcommand)
          {
            fprintf(stderr, "%s: Cannot specify more than one sub-command.\n", base_name);
            return (1);
          }

          subcommand = argv[i];
          break;
	}
      }

      if (j >= (int)(sizeof(subcommands) / sizeof(subcommands[0])))
      {
        // Not a standard sub-command...
	if (subcmd_name && !strcmp(argv[i], subcmd_name))
        {
          // Extension sub-command...
          if (subcommand)
          {
            fprintf(stderr, "%s: Cannot specify more than one sub-command.\n", base_name);
            return (1);
          }

          subcommand = argv[i];
        }
        else if (num_files < (int)(sizeof(files) / sizeof(files[0])))
        {
          // Filename...
	  files[num_files ++] = argv[i];
        }
        else
	{
	  // Too many files...
	  fprintf(stderr, "%s: Too many files.\n", base_name);
	  return (1);
	}
      }
    }
    else if (argv[i][0] == '-')
    {
      for (opt = argv[i] + 1; *opt; opt ++)
      {
	switch (argv[i][1])
	{
          case 'a': // -a (cancel-all)
              num_options = cupsAddOption("cancel-all", "true", num_options, &options);
              break;

          case 'd': // -d PRINTER
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing printer name after '-d'.\n", base_name);
                return (1);
              }

              num_options = cupsAddOption("printer-name", argv[i], num_options, &options);
	      break;

          case 'h': // -d HOST
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing hostname after '-h'.\n", base_name);
                return (1);
              }

              num_options = cupsAddOption("server-hostname", argv[i], num_options, &options);
	      break;

          case 'j': // -j JOB-ID
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing job ID after '-j'.\n", base_name);
                return (1);
              }

              num_options = cupsAddOption("job-id", argv[i], num_options, &options);
	      break;

          case 'm': // -m DRIVER-NAME
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing driver name after '-m'.\n", base_name);
                return (1);
              }

              num_options = cupsAddOption("smi2699-device-command", argv[i], num_options, &options);
	      break;

          case 'n': // -n COPIES
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing copy count after '-n'.\n", base_name);
                return (1);
              }

              num_options = cupsAddOption("num-copies", argv[i], num_options, &options);
              break;

          case 'o': // -o "NAME=VALUE [... NAME=VALUE]"
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing option(s) after '-o'.\n", base_name);
                return (1);
              }

              num_options = cupsParseOptions(argv[i], num_options, &options);
	      break;

          case 't' : // -t TITLE
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing title after '-t'.\n", base_name);
                return (1);
	      }

              num_options = cupsAddOption("job-name", argv[i], num_options, &options);
              break;

          case 'u': // -u PRINTER-URI
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing printer URI after '-u'.\n", base_name);
                return (1);
              }

              num_options = cupsAddOption("printer-uri", argv[i], num_options, &options);
	      break;

          case 'v': // -v DEVICE-URI
              i ++;
              if (i >= argc)
              {
                fprintf(stderr, "%s: Missing device URI after '-v'.\n", base_name);
                return (1);
              }

              num_options = cupsAddOption("smi2699-device-uri", argv[i], num_options, &options);
	      break;

          default:
              fprintf(stderr, "%s: Unknown option '-%c'.\n", base_name, *opt);
              return (1);
        }
      }
    }
    else
    {
      if (!subcommand)
      {
        subcommand = argv[i];
      }
      else
      {
        if (num_files >= (int)(sizeof(files) / sizeof(files[0])))
        {
          printf("%s: Cannot print more files.\n", base_name);
          return (1);
        }
        files[num_files++] = argv[i];
      }
    }
  }

  // Process sub-commands
  if (!subcommand || !strcmp(subcommand, "submit"))
  {
    return (_papplMainloopSubmitJob(base_name, num_options, options, num_files, files));
  }
  else if (subcmd_name && !strcmp(subcommand, subcmd_name))
  {
    return ((subcmd_cb)(base_name, num_options, options, num_files, files, data));
  }
  else if (num_files > 0)
  {
    fprintf(stderr, "%s: Sub-command '%s' does not accept files.\n", base_name, subcommand);
    return (1);
  }
  else if (!strcmp(subcommand, "add"))
  {
    return (_papplMainloopAddPrinter(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "auto-add"))
  {
    return (_papplMainloopAutoAddDevices(base_name, num_options, options, get_driver_cb));
  }
  else if (!strcmp(subcommand, "cancel"))
  {
    return (_papplMainloopCancelJob(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "default"))
  {
    return (_papplMainloopGetSetDefaultPrinter(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "delete"))
  {
    return (_papplMainloopDeletePrinter(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "devices"))
  {
    return (_papplMainloopShowDevices(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "drivers"))
  {
    return (_papplMainloopShowDrivers(base_name, num_options, options, system_cb, data));
  }
  else if (!strcmp(subcommand, "jobs"))
  {
    return (_papplMainloopShowJobs(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "modify"))
  {
    return (_papplMainloopModifyPrinter(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "options"))
  {
    return (_papplMainloopShowOptions(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "printers"))
  {
    return (_papplMainloopShowPrinters(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "server"))
  {
    return (_papplMainloopRunServer(base_name, num_options, options, system_cb, data));
  }
  else if (!strcmp(subcommand, "shutdown"))
  {
    return (_papplMainloopShutdownServer(base_name, num_options, options));
  }
  else if (!strcmp(subcommand, "status"))
  {
    return (_papplMainloopShowStatus(base_name, num_options, options));
  }
  else
  {
    // This should never happen...
    fprintf(stderr, "%s: Unknown sub-command '%s'.\n", base_name, subcommand);
    return (1);
  }
}


//
// 'usage()' - Show default usage.
//

static void
usage(const char *base_name)		// I - Base name of application
{
  printf("Usage: %s SUB-COMMAND [OPTIONS] [FILENAME]\n", base_name);
  printf("       %s [OPTIONS] [FILENAME]\n", base_name);
  printf("       %s [OPTIONS] -\n", base_name);
  puts("");
  puts("Sub-commands:");
  puts("  add PRINTER      Add a printer.");
  puts("  auto-add         Automatically add printers.");
  puts("  cancel           Cancel one or more jobs.");
  puts("  default          Set the default printer.");
  puts("  delete           Delete a printer.");
  puts("  devices          List devices.");
  puts("  drivers          List drivers.");
  puts("  jobs             List jobs.");
  puts("  modify           Modify a printer.");
  puts("  options          List printer options.");
  puts("  printers         List printers.");
  puts("  server           Run a server.");
  puts("  shutdown         Shutdown a running server.");
  puts("  status           Show server/printer/job status.");
  puts("  submit           Submit a file for printing.");
  puts("");
  puts("Options:");
  puts("  -a               Cancel all jobs (cancel).");
  puts("  -d PRINTER       Specify printer.");
  puts("  -j JOB-ID        Specify job ID (cancel).");
  puts("  -m DRIVER-NAME   Specify driver (add/modify).");
  puts("  -n COPIES        Specify number of copies (submit).");
  puts("  -o NAME=VALUE    Specify option (add,modify,server,submit).");
  puts("  -u URI           Specify ipp: or ipps: printer/server.");
  puts("  -v DEVICE-URI    Specify socket: or usb: device (add/modify).");
}
