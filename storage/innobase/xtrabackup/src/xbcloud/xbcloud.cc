/******************************************************
Copyright (c) 2014, 2021 Percona LLC and/or its affiliates.

xbcloud utility. Manage backups on cloud storage services.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

*******************************************************/

#include <my_default.h>
#include <my_dir.h>
#include <my_getopt.h>
#include <my_sys.h>
#include <mysql/service_mysql_alloc.h>
#include <signal.h>
#include <typelib.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>

#include <curl/curl.h>

#include "common.h"
#include "xbstream.h"
#include "xtrabackup_version.h"

#include "crc_glue.h"

#include "xbcloud/azure.h"
#include "xbcloud/s3.h"
#include "xbcloud/swift.h"
#include "xbcloud/util.h"
#include "xbcrypt_common.h"

using namespace xbcloud;

#define XBCLOUD_VERSION XTRABACKUP_VERSION
#define XBCLOUD_REVISION XTRABACKUP_REVISION

/*****************************************************************************/

const char *config_file = "my"; /* Default config file */

const char *azure_development_access_key =
    "Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/"
    "KBHBeksoGMGw==";
const char *azure_development_storage_account = "devstoreaccount1";
const char *azure_development_container = "testcontainer";

enum { SWIFT, S3, GOOGLE, AZURE };
const char *storage_names[] = {"SWIFT", "S3", "GOOGLE", "AZURE", NullS};

const char *s3_bucket_lookup_names[] = {"AUTO", "DNS", "PATH", NullS};

static bool opt_verbose = 0;

static ulong opt_storage = SWIFT;

static char *opt_swift_user = nullptr;
static char *opt_swift_user_id = nullptr;
static char *opt_swift_password = nullptr;
static char *opt_swift_tenant = nullptr;
static char *opt_swift_tenant_id = nullptr;
static char *opt_swift_project = nullptr;
static char *opt_swift_project_id = nullptr;
static char *opt_swift_domain = nullptr;
static char *opt_swift_domain_id = nullptr;
static char *opt_swift_project_domain = nullptr;
static char *opt_swift_project_domain_id = nullptr;
static char *opt_swift_region = nullptr;
static char *opt_swift_container = nullptr;
static char *opt_swift_storage_url = nullptr;
static char *opt_swift_auth_url = nullptr;
static char *opt_swift_key = nullptr;
static char *opt_swift_auth_version = nullptr;

static char *opt_s3_region = nullptr;
static char *opt_s3_endpoint = nullptr;
static char *opt_s3_access_key = nullptr;
static char *opt_s3_secret_key = nullptr;
static char *opt_s3_session_token = nullptr;
static char *opt_s3_storage_class = nullptr;
static char *opt_s3_bucket = nullptr;
static ulong opt_s3_bucket_lookup;
static ulong opt_s3_api_version = 0;

static char *opt_google_region = nullptr;
static char *opt_google_endpoint = nullptr;
static char *opt_google_access_key = nullptr;
static char *opt_google_secret_key = nullptr;
static char *opt_google_session_token = nullptr;
static char *opt_google_storage_class = nullptr;
static char *opt_google_bucket = nullptr;

static char *opt_azure_account = nullptr;
static char *opt_azure_container = nullptr;
static char *opt_azure_access_key = nullptr;
static char *opt_azure_endpoint = nullptr;
static char *opt_azure_storage_class = nullptr;
static bool opt_azure_development_storage = 0;

static std::string backup_name;
static char *opt_cacert = nullptr;
static ulong opt_parallel = 1;
static ulong opt_max_retries = 10;
static u_int32_t opt_max_backoff = 300000;

static bool opt_insecure = false;
static bool opt_md5 = false;
static enum { MODE_GET, MODE_PUT, MODE_DELETE } opt_mode;

static std::map<std::string, std::string> extra_http_headers;

const char *s3_api_version_names[] = {"AUTO", "2", "4", NullS};

static std::set<std::string> file_list;

TYPELIB storage_typelib = {array_elements(storage_names) - 1, "", storage_names,
                           nullptr};

TYPELIB s3_bucket_lookup_typelib = {array_elements(s3_bucket_lookup_names) - 1,
                                    "", s3_bucket_lookup_names, nullptr};

TYPELIB s3_api_version_typelib = {array_elements(s3_api_version_names) - 1, "",
                                  s3_api_version_names, nullptr};

Http_client http_client;

enum {
  OPT_STORAGE = 256,

  OPT_SWIFT_CONTAINER,
  OPT_SWIFT_AUTH_URL,
  OPT_SWIFT_KEY,
  OPT_SWIFT_USER,
  OPT_SWIFT_USER_ID,
  OPT_SWIFT_PASSWORD,
  OPT_SWIFT_TENANT,
  OPT_SWIFT_TENANT_ID,
  OPT_SWIFT_PROJECT,
  OPT_SWIFT_PROJECT_ID,
  OPT_SWIFT_DOMAIN,
  OPT_SWIFT_DOMAIN_ID,
  OPT_SWIFT_PROJECT_DOMAIN,
  OPT_SWIFT_PROJECT_DOMAIN_ID,
  OPT_SWIFT_REGION,
  OPT_SWIFT_STORAGE_URL,
  OPT_SWIFT_AUTH_VERSION,

  OPT_S3_REGION,
  OPT_S3_ENDPOINT,
  OPT_S3_ACCESS_KEY,
  OPT_S3_SECRET_KEY,
  OPT_S3_SESSION_TOKEN,
  OPT_S3_STORAGE_CLASS,
  OPT_S3_BUCKET,
  OPT_S3_BUCKET_LOOKUP,
  OPT_S3_API_VERSION,

  OPT_AZURE_ACCOUNT,
  OPT_AZURE_CONTAINER,
  OPT_AZURE_ACCESS_KEY,
  OPT_AZURE_STORAGE_CLASS,
  OPT_AZURE_ENDPOINT,
  OPT_AZURE_DEVELOPMENT_STORAGE,

  OPT_GOOGLE_REGION,
  OPT_GOOGLE_ENDPOINT,
  OPT_GOOGLE_ACCESS_KEY,
  OPT_GOOGLE_SECRET_KEY,
  OPT_GOOGLE_SESSION_TOKEN,
  OPT_GOOGLE_STORAGE_CLASS,
  OPT_GOOGLE_BUCKET,

  OPT_PARALLEL,
  OPT_MAX_RETRIES,
  OPT_MAX_BACKOFF,
  OPT_CACERT,
  OPT_HEADER,
  OPT_INSECURE,
  OPT_MD5,
  OPT_VERBOSE,
  OPT_CURL_RETRIABLE_ERRORS,
  OPT_HTTP_RETRIABLE_ERRORS
};

static struct my_option my_long_options[] = {
    {"defaults-file", 'c',
     "Name of config file to read; if no extension is given, default "
     "extension (e.g., .ini or .cnf) will be added",
     &config_file, &config_file, 0, GET_STR, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"help", '?', "Display this help and exit.", 0, 0, 0, GET_NO_ARG, NO_ARG, 0,
     0, 0, 0, 0, 0},

    {"version", 'V', "Display version and exit.", 0, 0, 0, GET_NO_ARG, NO_ARG,
     0, 0, 0, 0, 0, 0},

    {"storage", OPT_STORAGE, "Specify storage type S3/SWIFT/GOOGLE/AZURE.",
     &opt_storage, &opt_storage, &storage_typelib, GET_ENUM, REQUIRED_ARG, 0, 0,
     0, 0, 0, 0},

    {"swift-auth-version", OPT_SWIFT_AUTH_VERSION,
     "Swift authentication verison to use.", &opt_swift_auth_version,
     &opt_swift_auth_version, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"swift-container", OPT_SWIFT_CONTAINER,
     "Swift container to store backups into.", &opt_swift_container,
     &opt_swift_container, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"swift-user", OPT_SWIFT_USER, "Swift user name.", &opt_swift_user,
     &opt_swift_user, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"swift-user-id", OPT_SWIFT_USER_ID, "Swift user ID.", &opt_swift_user_id,
     &opt_swift_user_id, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"swift-auth-url", OPT_SWIFT_AUTH_URL,
     "Base URL of SWIFT authentication service.", &opt_swift_auth_url,
     &opt_swift_auth_url, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"swift-storage-url", OPT_SWIFT_STORAGE_URL,
     "URL of object-store endpoint. Usually received from authentication "
     "service. Specify to override this value.",
     &opt_swift_storage_url, &opt_swift_storage_url, 0, GET_STR_ALLOC,
     REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"swift-key", OPT_SWIFT_KEY, "Swift key.", &opt_swift_key, &opt_swift_key,
     0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"swift-tenant", OPT_SWIFT_TENANT,
     "The tenant name. Both the --swift-tenant and --swift-tenant-id "
     "options are optional, but should not be specified together.",
     &opt_swift_tenant, &opt_swift_tenant, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0,
     0, 0, 0, 0},

    {"swift-tenant-id", OPT_SWIFT_TENANT_ID,
     "The tenant ID. Both the --swift-tenant and --swift-tenant-id "
     "options are optional, but should not be specified together.",
     &opt_swift_tenant_id, &opt_swift_tenant_id, 0, GET_STR_ALLOC, REQUIRED_ARG,
     0, 0, 0, 0, 0, 0},

    {"swift-project", OPT_SWIFT_PROJECT, "The project name.",
     &opt_swift_project, &opt_swift_project, 0, GET_STR_ALLOC, REQUIRED_ARG, 0,
     0, 0, 0, 0, 0},

    {"swift-project-id", OPT_SWIFT_PROJECT_ID, "The project ID.",
     &opt_swift_project_id, &opt_swift_project_id, 0, GET_STR_ALLOC,
     REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"swift-domain", OPT_SWIFT_DOMAIN, "The user domain name.",
     &opt_swift_domain, &opt_swift_domain, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0,
     0, 0, 0, 0},

    {"swift-domain-id", OPT_SWIFT_DOMAIN_ID, "The user domain ID.",
     &opt_swift_domain_id, &opt_swift_domain_id, 0, GET_STR_ALLOC, REQUIRED_ARG,
     0, 0, 0, 0, 0, 0},

    {"swift-project-domain", OPT_SWIFT_PROJECT_DOMAIN,
     "The project domain name.", &opt_swift_project_domain,
     &opt_swift_project_domain, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0,
     0},

    {"swift-project-domain-id", OPT_SWIFT_PROJECT_DOMAIN_ID,
     "The project domain ID.", &opt_swift_project_domain_id,
     &opt_swift_project_domain_id, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0,
     0, 0},

    {"swift-password", OPT_SWIFT_PASSWORD, "The password of the user.",
     &opt_swift_password, &opt_swift_password, 0, GET_STR_ALLOC, REQUIRED_ARG,
     0, 0, 0, 0, 0, 0},

    {"swift-region", OPT_SWIFT_REGION, "The region object-store endpoint.",
     &opt_swift_region, &opt_swift_region, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0,
     0, 0, 0, 0},

    {"parallel", OPT_PARALLEL, "Number of parallel chunk uploads.",
     &opt_parallel, &opt_parallel, 0, GET_ULONG, REQUIRED_ARG, 1, 1, ULONG_MAX,
     0, 0, 0},

    {"max-retries", OPT_MAX_RETRIES,
     "Number of retries of chunk uploads/downloads after a failure (Default "
     "10).",
     &opt_max_retries, &opt_max_retries, 0, GET_ULONG, REQUIRED_ARG, 10, 1,
     ULONG_MAX, 0, 0, 0},

    {"max-backoff", OPT_MAX_BACKOFF,
     "Maximum backoff delay in milliseconds in between chunk uploads/downloads "
     "retries "
     "(Default 300000).",
     &opt_max_backoff, &opt_max_backoff, 0, GET_UINT32, REQUIRED_ARG, 300000, 1,
     UINT_MAX32, 0, 0, 0},

    {"s3-region", OPT_S3_REGION, "S3 region.", &opt_s3_region, &opt_s3_region,
     0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"s3-endpoint", OPT_S3_ENDPOINT, "S3 endpoint.", &opt_s3_endpoint,
     &opt_s3_endpoint, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"s3-access-key", OPT_S3_ACCESS_KEY, "S3 access key.", &opt_s3_access_key,
     &opt_s3_access_key, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"s3-secret-key", OPT_S3_SECRET_KEY, "S3 secret key.", &opt_s3_secret_key,
     &opt_s3_secret_key, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"s3-session-token", OPT_S3_SESSION_TOKEN, "S3 session token.",
     &opt_s3_session_token, &opt_s3_session_token, 0, GET_STR_ALLOC,
     REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"s3-storage-class", OPT_S3_STORAGE_CLASS,
     "S3 storage class. STANDARD|STANDARD_IA|GLACIER|...     "
     "... is meant for passing "
     "custom storage class names provided by other S3 implementations such as "
     "MinIO CephRadosGW, etc.",
     &opt_s3_storage_class, &opt_s3_storage_class, 0, GET_STR_ALLOC,
     REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"s3-bucket", OPT_S3_BUCKET, "S3 bucket.", &opt_s3_bucket, &opt_s3_bucket,
     0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"s3-bucket-lookup", OPT_S3_BUCKET_LOOKUP, "Bucket lookup method.",
     &opt_s3_bucket_lookup, &opt_s3_bucket_lookup, &s3_bucket_lookup_typelib,
     GET_ENUM, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"s3-api-version", OPT_S3_API_VERSION, "S3 API version.",
     &opt_s3_api_version, &opt_s3_api_version, &s3_api_version_typelib,
     GET_ENUM, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"azure-storage-account", OPT_AZURE_ACCOUNT, "AZURE storage account. ",
     &opt_azure_account, &opt_azure_account, 0, GET_STR_ALLOC, REQUIRED_ARG, 0,
     0, 0, 0, 0, 0},

    {"azure-container-name", OPT_AZURE_CONTAINER, "AZURE container name. ",
     &opt_azure_container, &opt_azure_container, 0, GET_STR_ALLOC, REQUIRED_ARG,
     0, 0, 0, 0, 0, 0},

    {"azure-access-key", OPT_AZURE_ACCESS_KEY, "AZURE access key.",
     &opt_azure_access_key, &opt_azure_access_key, 0, GET_STR_ALLOC,
     REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"azure-development-storage", OPT_AZURE_DEVELOPMENT_STORAGE,
     "To run against azurite emulator use --azure-development-storage. It can "
     "work with the default credentials provided by azurite. For example, it "
     "uses http://127.0.0.1:10000 as the default endpoint, which can be "
     "overwritten by --azure-endpoint. Users can also provide "
     "--azure-access-key,  --azure-storage-account, --azure-container-name",
     &opt_azure_development_storage, &opt_azure_development_storage, 0,
     GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},

    {"azure-endpoint", OPT_AZURE_ENDPOINT, "Azure cloud storage endpoint.",
     &opt_azure_endpoint, &opt_azure_endpoint, 0, GET_STR_ALLOC, REQUIRED_ARG,
     0, 0, 0, 0, 0, 0},

    {"azure-tier-class", OPT_AZURE_STORAGE_CLASS,
     "Azure cloud tier class. Hot|Cool|Archive", &opt_azure_storage_class,
     &opt_azure_storage_class, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0,
     0},

    {"google-region", OPT_GOOGLE_REGION, "Google cloud storage region.",
     &opt_google_region, &opt_google_region, 0, GET_STR_ALLOC, REQUIRED_ARG, 0,
     0, 0, 0, 0, 0},

    {"google-endpoint", OPT_GOOGLE_ENDPOINT, "Google cloud storage endpoint.",
     &opt_google_endpoint, &opt_google_endpoint, 0, GET_STR_ALLOC, REQUIRED_ARG,
     0, 0, 0, 0, 0, 0},

    {"google-access-key", OPT_GOOGLE_ACCESS_KEY,
     "Google cloud storage access key.", &opt_google_access_key,
     &opt_google_access_key, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"google-secret-key", OPT_GOOGLE_SECRET_KEY,
     "Google cloud storage secret key.", &opt_google_secret_key,
     &opt_google_secret_key, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"google-session-token", OPT_GOOGLE_SESSION_TOKEN,
     "Google cloud storage session token.", &opt_google_session_token,
     &opt_google_session_token, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0,
     0},

    {"google-storage-class", OPT_GOOGLE_STORAGE_CLASS,
     "Google cloud storage class. STANDARD|NEARLINE|COLDLINE|ARCHIVE",
     &opt_google_storage_class, &opt_google_storage_class, 0, GET_STR_ALLOC,
     REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"google-bucket", OPT_GOOGLE_BUCKET, "Google cloud storage bucket.",
     &opt_google_bucket, &opt_google_bucket, 0, GET_STR_ALLOC, REQUIRED_ARG, 0,
     0, 0, 0, 0, 0},

    {"cacert", OPT_CACERT, "CA certificate file.", &opt_cacert, &opt_cacert, 0,
     GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"header", OPT_HEADER, "Extra header.", NULL, NULL, 0, GET_STR,
     REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"insecure", OPT_INSECURE, "Do not verify server SSL certificate.",
     &opt_insecure, &opt_insecure, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},

    {"md5", OPT_MD5, "Upload MD5 file into the backup dir.", &opt_md5, &opt_md5,
     0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},

    {"verbose", OPT_VERBOSE, "Turn ON cURL tracing.", &opt_verbose,
     &opt_verbose, 0, GET_BOOL, NO_ARG, 0, 0, 0, 0, 0, 0},

    {"curl-retriable-errors", OPT_CURL_RETRIABLE_ERRORS,
     "Add a new curl error code as retriable. For multiple codes, use a comma "
     "separated list of codes.",
     0, 0, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {"http-retriable-errors", OPT_HTTP_RETRIABLE_ERRORS,
     "Add a new http error code as retriable. For multiple codes, use a comma "
     "separated list of codes.",
     0, 0, 0, GET_STR_ALLOC, REQUIRED_ARG, 0, 0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0, 0, GET_NO_ARG, NO_ARG, 0, 0, 0, 0, 0, 0}};

static void print_version() {
  printf("%s  Ver %s for %s (%s) (revision id: %s)\n", my_progname, XBCLOUD_VERSION, SYSTEM_TYPE,
         MACHINE_TYPE, XBCLOUD_REVISION);
}

static void usage() {
  print_version();
  puts("Copyright (C) 2015, 2021 Percona LLC and/or its affiliates.");
  puts(
      "This software comes with ABSOLUTELY NO WARRANTY. "
      "This is free software,\nand you are welcome to modify and "
      "redistribute it under the GPL license.\n");

  puts("Manage backups on the cloud services.\n");

  puts("Usage: ");
  printf(
      "  %s put [OPTIONS...] <NAME> upload backup from STDIN into "
      "the cloud service with given name.\n",
      my_progname);
  printf(
      "  %s get [OPTIONS...] <NAME> [FILES...] stream specified "
      "backup or individual files from the cloud service into STDOUT.\n",
      my_progname);
  printf(
      "  %s delete [OPTIONS...] <NAME> [FILES...] delete specified "
      "backup or individual files from the cloud service.\n",
      my_progname);

  puts("\nOptions:");
  my_print_help(my_long_options);
}

static my_bool get_one_option(int optid,
                              const struct my_option *opt
                              __attribute__((unused)),
                              char *argument) {
  switch (optid) {
    case '?':
      usage();
      exit(0);
    case 'V':
      print_version();
      exit(0);
    case OPT_SWIFT_PASSWORD:
    case OPT_SWIFT_KEY:
    case OPT_SWIFT_TENANT:
    case OPT_SWIFT_TENANT_ID:
    case OPT_S3_ACCESS_KEY:
    case OPT_S3_SECRET_KEY:
    case OPT_S3_SESSION_TOKEN:
    case OPT_GOOGLE_ACCESS_KEY:
    case OPT_GOOGLE_SECRET_KEY:
    case OPT_GOOGLE_SESSION_TOKEN:
    case OPT_AZURE_ACCESS_KEY:
    case OPT_AZURE_ACCOUNT:
      if (argument != nullptr) {
        while (*argument) *argument++ = 0;  // Destroy argument
      }
      break;
    case OPT_HEADER:
      if (argument != nullptr) {
        extra_http_headers.insert(parse_http_header(argument));
      }
      break;
    case OPT_CURL_RETRIABLE_ERRORS:
      if (argument != nullptr) {
        std::istringstream iss(argument);
        for (std::string val; std::getline(iss, val, ',');) {
          char *ptr;
          long int code = strtol(val.c_str(), &ptr, 10);
          if (!*ptr) {
            http_client.set_curl_retriable_errors(
                std::move(static_cast<CURLcode>(code)));
          }
        }
      }
      break;
    case OPT_HTTP_RETRIABLE_ERRORS:
      if (argument != nullptr) {
        std::istringstream iss(argument);
        for (std::string val; std::getline(iss, val, ',');) {
          char *ptr;
          long int code = strtol(val.c_str(), &ptr, 10);
          if (!*ptr) {
            http_client.set_http_retriable_errors(std::move(code));
          }
        }
      }
      break;
  }

  return (false);
}

static const char *load_default_groups[] = {"xbcloud", 0};

static void get_env_value(char *&var, const char *env) {
  char *val;
  if (var == nullptr && (val = getenv(env)) != nullptr) {
    var = my_strdup(PSI_NOT_INSTRUMENTED, val, MYF(MY_FAE));
  }
}

static void get_env_args() {
  get_env_value(opt_swift_auth_url, "OS_AUTH_URL");
  get_env_value(opt_swift_tenant, "OS_TENANT_NAME");
  get_env_value(opt_swift_tenant_id, "OS_TENANT_ID");
  get_env_value(opt_swift_user, "OS_USERNAME");
  get_env_value(opt_swift_password, "OS_PASSWORD");
  get_env_value(opt_swift_domain, "OS_USER_DOMAIN");
  get_env_value(opt_swift_domain_id, "OS_USER_DOMAIN_ID");
  get_env_value(opt_swift_project_domain, "OS_PROJECT_DOMAIN");
  get_env_value(opt_swift_project_domain_id, "OS_PROJECT_DOMAIN_ID");
  get_env_value(opt_swift_region, "OS_REGION_NAME");
  get_env_value(opt_swift_storage_url, "OS_STORAGE_URL");
  get_env_value(opt_cacert, "OS_CACERT");

  /* Below block should always be above AWS_* and should not be moved because
  the order of prefrence are like S3_ACCESS_KEY_ID, AWS_ACCESS_KEY_ID and
  ACCESS_KEY  */
  get_env_value(opt_s3_access_key, "S3_ACCESS_KEY_ID");
  get_env_value(opt_s3_secret_key, "S3_SECRET_ACCESS_KEY");
  get_env_value(opt_s3_session_token, "S3_SESSION_TOKEN");
  get_env_value(opt_s3_storage_class, "S3_STORAGE_CLASS");
  get_env_value(opt_s3_region, "S3_DEFAULT_REGION");
  get_env_value(opt_cacert, "S3_CA_BUNDLE");
  get_env_value(opt_s3_endpoint, "S3_ENDPOINT");

  get_env_value(opt_s3_access_key, "AWS_ACCESS_KEY_ID");
  get_env_value(opt_s3_secret_key, "AWS_SECRET_ACCESS_KEY");
  get_env_value(opt_s3_session_token, "AWS_SESSION_TOKEN");
  get_env_value(opt_s3_storage_class, "AWS_STORAGE_CLASS");
  get_env_value(opt_s3_region, "AWS_DEFAULT_REGION");
  get_env_value(opt_cacert, "AWS_CA_BUNDLE");
  get_env_value(opt_s3_endpoint, "AWS_ENDPOINT");

  get_env_value(opt_s3_access_key, "ACCESS_KEY_ID");
  get_env_value(opt_s3_secret_key, "SECRET_ACCESS_KEY");
  get_env_value(opt_s3_region, "DEFAULT_REGION");
  get_env_value(opt_s3_endpoint, "ENDPOINT");

  get_env_value(opt_azure_account, "AZURE_STORAGE_ACCOUNT");
  get_env_value(opt_azure_container, "AZURE_CONTAINER_NAME");
  get_env_value(opt_azure_access_key, "AZURE_ACCESS_KEY");
  get_env_value(opt_azure_storage_class, "AZURE_STORAGE_CLASS");
  get_env_value(opt_azure_endpoint, "AZURE_ENDPOINT");

  get_env_value(opt_google_access_key, "ACCESS_KEY_ID");
  get_env_value(opt_google_secret_key, "SECRET_ACCESS_KEY");
  get_env_value(opt_google_session_token, "SESSION_TOKEN");
  get_env_value(opt_google_region, "DEFAULT_REGION");
  get_env_value(opt_google_endpoint, "ENDPOINT");
}

static char **defaults_argv = nullptr;

static bool parse_args(int argc, char **argv) {
  if (load_defaults("my", load_default_groups, &argc,
                    &argv /*, &argv_alloc*/)) {
    return true;
  }

  defaults_argv = argv;

  if (handle_options(&argc, &argv, my_long_options, get_one_option)) {
    return true;
  }

  const char *command;

  if (argc < 1) {
    msg_ts("Command isn't specified. Supported commands are put and get\n");
    usage();
    return true;
  }

  command = argv[0];
  argc--;
  argv++;

  get_env_args();

  if (strcasecmp(command, "put") == 0) {
    opt_mode = MODE_PUT;
  } else if (strcasecmp(command, "get") == 0) {
    opt_mode = MODE_GET;
  } else if (strcasecmp(command, "delete") == 0) {
    opt_mode = MODE_DELETE;
  } else {
    msg_ts("Unknown command %s. Supported commands are put and get\n", command);
    usage();
    return true;
  }

  /* make sure name is specified */
  if (argc < 1) {
    msg_ts("Backup name is required argument\n");
    return true;
  }
  backup_name = argv[0];
  argc--;
  argv++;

  std::string backup_uri = backup_name;
  std::string bucket_name;
  bool backup_name_uri_formatted = false;

  /* parse the backup name */
  if (starts_with(backup_uri, "s3://")) {
    backup_name_uri_formatted = true;
    opt_storage = S3;
    backup_uri = backup_uri.substr(5);
  } else if (starts_with(backup_uri, "google://")) {
    backup_name_uri_formatted = true;
    opt_storage = GOOGLE;
    backup_uri = backup_uri.substr(9);
  } else if (starts_with(backup_uri, "swift://")) {
    backup_name_uri_formatted = true;
    opt_storage = SWIFT;
    backup_uri = backup_uri.substr(8);
  } else if (starts_with(backup_uri, "azure://")) {
    backup_name_uri_formatted = true;
    opt_storage = AZURE;
    backup_uri = backup_uri.substr(8);
  }
  if (backup_name_uri_formatted) {
    auto slash_pos = backup_uri.find('/');
    if (slash_pos != std::string::npos) {
      bucket_name = backup_uri.substr(0, slash_pos);
      backup_name = backup_uri.substr(slash_pos + 1);
      rtrim_slashes(backup_name);
      if (opt_storage == S3) {
        my_free(opt_s3_bucket);
        opt_s3_bucket =
            my_strdup(PSI_NOT_INSTRUMENTED, bucket_name.c_str(), MYF(MY_WME));
      } else if (opt_storage == GOOGLE) {
        my_free(opt_google_bucket);
        opt_google_bucket =
            my_strdup(PSI_NOT_INSTRUMENTED, bucket_name.c_str(), MYF(MY_WME));
      } else if (opt_storage == SWIFT) {
        my_free(opt_swift_container);
        opt_swift_container =
            my_strdup(PSI_NOT_INSTRUMENTED, bucket_name.c_str(), MYF(MY_WME));
      } else if (opt_storage == AZURE) {
        my_free(opt_azure_container);
        opt_azure_container =
            my_strdup(PSI_NOT_INSTRUMENTED, bucket_name.c_str(), MYF(MY_WME));
      }
    }
  }

  if (opt_azure_development_storage) {
    opt_storage = AZURE;
  }

  /* validate arguments */
  if (opt_storage == SWIFT) {
    if (opt_swift_user == nullptr) {
      msg_ts("Swift user is not specified\n");
      return true;
    }
    if (opt_swift_container == nullptr) {
      msg_ts("Swift container is not specified\n");
      return true;
    }
    if (opt_swift_auth_url == nullptr) {
      msg_ts("Swift auth URL is not specified\n");
      return true;
    }
  } else if (opt_storage == S3) {
    if (opt_s3_access_key == nullptr) {
      msg_ts("S3 access key is not specified\n");
      return true;
    }
    if (opt_s3_secret_key == nullptr) {
      msg_ts("S3 secret key is not specified\n");
      return true;
    }
  } else if (opt_storage == AZURE) {
    if (opt_azure_account == nullptr && !opt_azure_development_storage) {
      msg_ts("Azure account name is not specified\n");
      return true;
    }
  }

  while (argc > 0) {
    file_list.insert(*argv);
    --argc;
    ++argv;
  }

  return (0);
}

bool xbcloud_put(Object_store *store, const std::string &container,
                 const std::string &backup_name, bool upload_md5) {
  bool exists;

  if (!store->container_exists(container, exists)) {
    return false;
  }

  if (!exists) {
    if (!store->create_container(container)) {
      return false;
    }
  }

  std::vector<std::string> object_list;
  if (!store->list_objects_in_directory(container, backup_name, object_list)) {
    return false;
  }

  if (!object_list.empty()) {
    msg_ts("%s: error: backup named %s already exists!\n", my_progname,
           backup_name.c_str());
    return false;
  }

  struct file_entry_t {
    my_off_t chunk_idx;
    my_off_t offset;
    std::string path;
  };

  std::unordered_map<std::string, std::unique_ptr<file_entry_t>> filehash;
  xb_rstream_t *stream = xb_stream_read_new();
  if (stream == nullptr) {
    msg_ts("%s: xb_stream_read_new() failed.", my_progname);
    return false;
  }

  xb_rstream_chunk_t chunk;
  xb_rstream_result_t res;

  bool has_errors = false;

  memset(&chunk, 0, sizeof(chunk));

  Http_buffer buf_md5;

  Event_handler h(opt_parallel > 0 ? opt_parallel : 1);
  if (!h.init()) {
    msg_ts("%s: Failed to initialize event handler.\n", my_progname);
    return false;
  }
  auto thread = h.run();

  do {
    res = xb_stream_read_chunk(stream, &chunk);
    if (res != XB_STREAM_READ_CHUNK) {
      my_free(chunk.raw_data);
      break;
    }
    if (chunk.type == XB_CHUNK_TYPE_UNKNOWN &&
        !(chunk.flags & XB_STREAM_FLAG_IGNORABLE)) {
      continue;
    }

    file_entry_t *entry = filehash[chunk.path].get();
    if (entry == nullptr) {
      entry = (filehash[chunk.path] = make_unique<file_entry_t>()).get();
      entry->path = chunk.path;
    }

    if (chunk.type == XB_CHUNK_TYPE_PAYLOAD) {
      res = (xb_rstream_result_t)xb_stream_validate_checksum(&chunk);
      if (res != XB_STREAM_READ_CHUNK) {
        break;
      }

      if (entry->offset != chunk.offset) {
        msg_ts(
            "%s: out-of-order chunk: real offset = 0x%llx, "
            "expected offset = 0x%llx\n",
            my_progname, chunk.offset, entry->offset);
        res = XB_STREAM_READ_ERROR;
        break;
      }
    }

    std::stringstream file_name_s;
    file_name_s << chunk.path << "." << std::setw(20) << std::setfill('0')
                << entry->chunk_idx;
    std::string file_name = file_name_s.str();
    std::string object_name = backup_name;
    object_name.append("/").append(file_name);

    Http_buffer buf = Http_buffer();
    buf.assign_buffer(static_cast<char *>(chunk.raw_data), chunk.buflen,
                      chunk.raw_length);

    if (upload_md5) {
      buf_md5.append(hex_encode(buf.md5()));
      buf_md5.append("  ");
      buf_md5.append(file_name);
      buf_md5.append("\n");
    }

    store->async_upload_object(
        container, object_name, buf, &h,
        std::bind(
            [](bool ok, std::string path, size_t length, bool *err) {
              if (ok) {
                msg_ts("%s: successfully uploaded chunk: %s, size: %zu\n",
                       my_progname, path.c_str(), length);
              } else {
                msg_ts("%s: error: failed to upload chunk: %s, size: %zu\n",
                       my_progname, path.c_str(), length);
                *err = true;
              }
            },
            std::placeholders::_1, object_name, chunk.raw_length, &has_errors));

    entry->offset += chunk.length;
    entry->chunk_idx++;

    if (chunk.type == XB_CHUNK_TYPE_EOF) {
      filehash.erase(entry->path);
    }

    memset(&chunk, 0, sizeof(chunk));
  } while (!has_errors);

  h.stop();
  thread.join();

  xb_stream_read_done(stream);


  // check if backup directory exists and it has some files
  if (res == XB_STREAM_READ_ERROR || has_errors ||
      !store->list_objects_in_directory(container, backup_name, object_list) ||
      object_list.size() == 0) {
    msg_ts("%s: Upload failed.\n", my_progname);
    return false;
  }

  /* check if the xtrabackup_info.gz.00000000000 or
  xtrabackup_info.00000000000000000000 is uploaded to cloud storage to
  determine successful xbcloud "put" operation. */
  bool file_found = false;
  if (!object_list.empty()) {
    std::string last_file_prefix = backup_name + "/xtrabackup_info";
    auto last_file_size = last_file_prefix.size();
    for (auto cur_file : object_list) {
      if (cur_file.size() >= last_file_size &&
	  cur_file.substr(0, last_file_size).compare(last_file_prefix) == 0) {
	  file_found = true;
        break;
      }
    }
  }
  if (!file_found) {
    msg_ts("%s: Upload failed: backup is incomplete.\n", my_progname);
    return false;
  }

  if (upload_md5) {
    msg_ts("%s: Uploading md5\n", my_progname);
    if (!store->upload_object(container, backup_name + ".md5", buf_md5)) {
      msg_ts("%s: Upload failed: Error uploading md5.\n", my_progname);
      return false;
    }
  }

  msg_ts("%s: Upload completed.\n", my_progname);
  return true;
}

bool chunk_name_to_file_name(const std::string &chunk_name,
                             std::string &file_name, my_off_t &idx) {
  if (chunk_name.size() < 22 && chunk_name[chunk_name.size() - 21] != '.') {
    /* chunk name is invalid */
    return false;
  }
  file_name = chunk_name.substr(0, chunk_name.size() - 21);
  idx = atoll(&chunk_name.c_str()[chunk_name.size() - 20]);
  return true;
}

bool xbcloud_delete(Object_store *store, const std::string &container,
                    const std::string &backup_name) {
  std::vector<std::string> object_list;

  if (!store->list_objects_in_directory(container, backup_name, object_list)) {
    msg_ts("%s: Delete failed. Cannot list %s.\n", my_progname,
           backup_name.c_str());
    return false;
  }
  if (object_list.empty()) {
    msg_ts("%s: error: backup named %s doesn't exists!\n", my_progname,
           backup_name.c_str());
    return false;
  }

  Event_handler h(opt_parallel > 0 ? opt_parallel : 1);
  if (!h.init()) {
    msg_ts("%s: Failed to initialize event handler.\n", my_progname);
    return false;
  }
  auto thread = h.run();

  bool error = false;
  for (const auto &obj : object_list) {
    std::string file_name;
    my_off_t idx;
    if (error) break;
    if (!chunk_name_to_file_name(obj, file_name, idx)) {
      continue;
    }
    if (!file_list.empty() &&
        file_list.count(file_name.substr(backup_name.length() + 1)) < 1) {
      continue;
    }
    msg_ts("%s: Deleting %s.\n", my_progname, obj.c_str());
    if (!store->async_delete_object(
            container, obj, &h,
            std::bind(
                [](bool success, std::string obj, bool *error) {
                  if (!success) {
                    msg_ts("%s: Delete failed. Cannot delete %s.\n",
                           my_progname, obj.c_str());
                    *error = true;
                  }
                },
                std::placeholders::_1, obj, &error))) {
      return false;
    }
  }

  h.stop();
  thread.join();

  if (error) {
    msg_ts("%s: Delete failed.\n", my_progname);
  } else {
    msg_ts("%s: Delete completed.\n", my_progname);
  }

  return !error;
}

bool xbcloud_download(Object_store *store, const std::string &container,
                      const std::string &backup_name) {
  std::vector<std::string> object_list;

  if (!store->list_objects_in_directory(container, backup_name, object_list) ||
      object_list.size() == 0) {
    msg_ts("%s: Download failed. Cannot list %s.\n", my_progname,
           backup_name.c_str());
    return false;
  }

  struct download_state {
    std::mutex m;
    std::set<std::string> chunks;

    bool empty() {
      std::lock_guard<std::mutex> g(m);
      return chunks.empty();
    }

    bool start_chunk(const std::string &chunk) {
      std::lock_guard<std::mutex> g(m);
      if (chunks.count(chunk) > 0) {
        return false;
      }
      chunks.insert(chunk);
      return true;
    }

    void complete_chunk(const std::string &chunk) {
      std::lock_guard<std::mutex> g(m);
      chunks.erase(chunk);
    }
  };

  struct chunk_state {
    my_off_t next;
    my_off_t last;
  };

  std::map<std::string, struct chunk_state> chunks;
  struct download_state download_state;

  for (const auto &obj : object_list) {
    my_off_t idx;
    std::string file_name;
    if (!chunk_name_to_file_name(obj, file_name, idx)) {
      continue;
    }
    if (!file_list.empty() &&
        file_list.count(file_name.substr(backup_name.length() + 1)) < 1) {
      continue;
    }
    chunks[file_name] = {0, idx};
  }

  Event_handler h(opt_parallel > 0 ? opt_parallel : 1);
  if (!h.init()) {
    msg_ts("%s: Failed to initialize event handler.\n", my_progname);
    return false;
  }
  auto thread = h.run();

  bool error{false};

  while (true) {
    if (chunks.empty() && download_state.empty()) {
      break;
    }
    if (error && !download_state.empty()) {
      continue;
    }
    if (error) break;
    for (auto it = chunks.begin(); it != chunks.end();) {
      if (error) break;
      if (!download_state.start_chunk(it->first)) {
        ++it;
        continue;
      }
      std::stringstream s_object_name;
      s_object_name << it->first << "." << std::setw(20) << std::setfill('0')
                    << it->second.next;
      std::string object_name = s_object_name.str();
      msg_ts("%s: Downloading %s.\n", my_progname, object_name.c_str());
      store->async_download_object(
          container, object_name, &h,
          std::bind(
              [](bool success, const Http_buffer &contents, std::string name,
                 std::string basename, struct download_state *download_state,
                 bool *error) {
                if (!success) {
                  *error = true;
                  msg_ts("%s: Download failed. Cannot download %s.\n",
                         my_progname, name.c_str());
                  download_state->complete_chunk(basename);
                  return;
                }
                msg_ts("%s: Download successfull %s, size %zu\n", my_progname,
                       name.c_str(), contents.size());
                std::cout.write(&contents[0], contents.size());
                std::cout.flush();
                download_state->complete_chunk(basename);
                if (std::cout.fail()) {
                  msg_ts(
                      "%s: Download failed. Cannot write to the standard "
                      "output.\n",
                      my_progname);
                  *error = true;
                }
              },
              std::placeholders::_1, std::placeholders::_2, object_name,
              it->first, &download_state, &error));
      if (++it->second.next > it->second.last) {
        it = chunks.erase(it);
      } else {
        ++it;
      }
    }
  }

  h.stop();
  thread.join();

  if (error) {
    msg_ts("%s: Download failed.\n", my_progname);
  } else {
    msg_ts("%s: Download completed.\n", my_progname);
  }

  return !error;
}

struct main_exit_hook {
  ~main_exit_hook() {
    if (defaults_argv != nullptr) {
      free_defaults(defaults_argv);
    }
    http_cleanup();
    my_end(0);
  }
};

int main(int argc, char **argv) {
  MY_INIT(argv[0]);

#ifndef NO_SIGPIPE
  signal(SIGPIPE, SIG_IGN);
#endif
  xb_libgcrypt_init();

  http_init();
  crc_init();

  /* trick to automatically release some globally alocated resources */
  main_exit_hook exit_hook;

  if (parse_args(argc, argv)) {
    return EXIT_FAILURE;
  }

  std::unique_ptr<Object_store> object_store = nullptr;
  if (opt_verbose) {
    http_client.set_verbose(true);
  }
  if (opt_insecure) {
    http_client.set_insecure(true);
  }
  if (opt_cacert != nullptr) {
    http_client.set_cacaert(opt_cacert);
  }

  std::string container_name;

  if (opt_storage == SWIFT) {
    std::string auth_url = opt_swift_auth_url;
    if (!ends_with(auth_url, "/")) {
      auth_url.append("/");
    }

    const char *valid_versions[] = {"/v1/",   "/v2/",   "/v3/",
                                    "/v1.0/", "/v2.0/", "/v3.0/"};
    bool versioned_url = false;
    for (auto version : valid_versions) {
      if (ends_with(opt_swift_auth_url, version)) {
        my_free(opt_swift_auth_version);
        opt_swift_auth_version =
            my_strdup(PSI_NOT_INSTRUMENTED,
                      std::string(version + 2, strlen(version + 2) - 1).c_str(),
                      MYF(MY_FAE));
        versioned_url = true;
      }
    }
    if (!versioned_url) {
      if (opt_swift_auth_version != nullptr) {
        auth_url.append("v");
        auth_url.append(opt_swift_auth_version);
        auth_url.append("/");
      } else {
        opt_swift_auth_version =
            my_strdup(PSI_NOT_INSTRUMENTED, "1.0", MYF(MY_FAE));
        auth_url.append("v");
        auth_url.append(opt_swift_auth_version);
        auth_url.append("/");
      }
    }

    Keystone_client keystone_client(&http_client, auth_url);

    if (opt_swift_key != nullptr) {
      keystone_client.set_key(opt_swift_key);
    }
    if (opt_swift_user != nullptr) {
      keystone_client.set_user(opt_swift_user);
    }
    if (opt_swift_password != nullptr) {
      keystone_client.set_password(opt_swift_password);
    }
    if (opt_swift_tenant != nullptr) {
      keystone_client.set_tenant(opt_swift_tenant);
    }
    if (opt_swift_tenant_id != nullptr) {
      keystone_client.set_tenant_id(opt_swift_tenant_id);
    }
    if (opt_swift_domain != nullptr) {
      keystone_client.set_domain(opt_swift_domain);
    }
    if (opt_swift_domain_id != nullptr) {
      keystone_client.set_domain_id(opt_swift_domain_id);
    }
    if (opt_swift_project_domain != nullptr) {
      keystone_client.set_project_domain(opt_swift_project_domain);
    }
    if (opt_swift_project_domain_id != nullptr) {
      keystone_client.set_project_domain_id(opt_swift_project_domain_id);
    }
    if (opt_swift_project != nullptr) {
      keystone_client.set_project(opt_swift_project);
    }
    if (opt_swift_project_id != nullptr) {
      keystone_client.set_project_id(opt_swift_project_id);
    }

    Keystone_client::auth_info_t auth_info;

    if (opt_swift_auth_version == NULL || *opt_swift_auth_version == '1') {
      /* TempAuth */
      if (!keystone_client.temp_auth(auth_info)) {
        return EXIT_FAILURE;
      }
    } else if (*opt_swift_auth_version == '2') {
      /* Keystone v2 */
      if (!keystone_client.auth_v2(
              opt_swift_region != nullptr ? opt_swift_region : "", auth_info)) {
        return EXIT_FAILURE;
      }
    } else if (*opt_swift_auth_version == '3') {
      /* Keystone v3 */
      if (!keystone_client.auth_v3(
              opt_swift_region != nullptr ? opt_swift_region : "", auth_info)) {
        return EXIT_FAILURE;
      }
    }

    if (opt_swift_storage_url != nullptr) {
      /* override storage url */
      auth_info.url = opt_swift_storage_url;
    }

    msg_ts("Object store URL: %s\n", auth_info.url.c_str());

    object_store = std::unique_ptr<Object_store>(
        new Swift_object_store(&http_client, auth_info.url, auth_info.token,
                               opt_max_retries, opt_max_backoff));

    container_name = opt_swift_container;

  } else if (opt_storage == S3) {
    std::string region =
        opt_s3_region != nullptr ? opt_s3_region : default_s3_region;
    std::string access_key =
        opt_s3_access_key != nullptr ? opt_s3_access_key : "";
    std::string secret_key =
        opt_s3_secret_key != nullptr ? opt_s3_secret_key : "";
    std::string session_token =
        opt_s3_session_token != nullptr ? opt_s3_session_token : "";
    std::string storage_class =
        opt_s3_storage_class != nullptr ? opt_s3_storage_class : "";
    object_store = std::unique_ptr<Object_store>(new S3_object_store(
        &http_client, region, access_key, secret_key, session_token,
        storage_class, opt_max_retries, opt_max_backoff,
        opt_s3_endpoint != nullptr ? opt_s3_endpoint : "",
        static_cast<s3_bucket_lookup_t>(opt_s3_bucket_lookup),
        static_cast<s3_api_version_t>(opt_s3_api_version)));

    if (opt_s3_bucket == nullptr) {
      msg_ts("%s: S3 bucket is not specified.\n", my_progname);
      return EXIT_FAILURE;
    }

    reinterpret_cast<S3_object_store *>(object_store.get())
        ->set_extra_http_headers(extra_http_headers);

    container_name = opt_s3_bucket;

    if (!reinterpret_cast<S3_object_store *>(object_store.get())
             ->probe_api_version_and_lookup(container_name)) {
      return EXIT_FAILURE;
    }
  } else if (opt_storage == GOOGLE) {
    std::string region =
        opt_google_region != nullptr ? opt_google_region : "us-central-1";
    std::string access_key =
        opt_google_access_key != nullptr ? opt_google_access_key : "";
    std::string secret_key =
        opt_google_secret_key != nullptr ? opt_google_secret_key : "";
    std::string session_token =
        opt_google_session_token != nullptr ? opt_google_session_token : "";
    std::string storage_class =
        opt_google_storage_class != nullptr ? opt_google_storage_class : "";

    object_store = std::unique_ptr<Object_store>(new S3_object_store(
        &http_client, region, access_key, secret_key, session_token,
        storage_class, opt_max_retries, opt_max_backoff,
        opt_google_endpoint != nullptr ? opt_google_endpoint
                                       : "https://storage.googleapis.com/",
        LOOKUP_DNS, S3_V4));

    if (opt_google_bucket == nullptr) {
      msg_ts("%s: Google bucket is not specified.\n", my_progname);
      return EXIT_FAILURE;
    }

    reinterpret_cast<S3_object_store *>(object_store.get())
        ->set_extra_http_headers(extra_http_headers);

    container_name = opt_google_bucket;

    if (!reinterpret_cast<S3_object_store *>(object_store.get())
             ->probe_api_version_and_lookup(container_name)) {
      return EXIT_FAILURE;
    }
  } else if (opt_storage == AZURE) {
    std::string storage_account =
        opt_azure_account != nullptr ? opt_azure_account : "";
    if (storage_account.empty() && opt_azure_development_storage) {
      storage_account.assign(azure_development_storage_account);
    }
    if (storage_account.empty()) {
      msg_ts("%s: Azure storage account is not specified.\n", my_progname);
      return EXIT_FAILURE;
    }

    std::string access_key =
        opt_azure_access_key != nullptr ? opt_azure_access_key : "";
    if (access_key.empty() && opt_azure_development_storage) {
      access_key.assign(azure_development_access_key);
    }
    if (access_key.empty()) {
      msg_ts("%s: Azure access key is not specified.\n", my_progname);
      return EXIT_FAILURE;
    }

    container_name = opt_azure_container != nullptr ? opt_azure_container : "";
    if (container_name.empty() && opt_azure_development_storage) {
      container_name.assign(azure_development_container);
    }
    if (container_name.empty()) {
      msg_ts("%s: Azure container is not specified.\n", my_progname);
      return EXIT_FAILURE;
    }

    std::string storage_class =
        opt_azure_storage_class != nullptr ? opt_azure_storage_class : "";

    std::string azure_endpoint =
        opt_azure_endpoint != nullptr ? opt_azure_endpoint : "";

    object_store = std::unique_ptr<Object_store>(new Azure_object_store(
        &http_client, storage_account, access_key,
        opt_azure_development_storage, storage_class, opt_max_retries,
        opt_max_backoff, azure_endpoint));

    reinterpret_cast<Azure_object_store *>(object_store.get())
        ->set_extra_http_headers(extra_http_headers);
  }

  if (opt_mode == MODE_PUT) {
    if (!xbcloud_put(object_store.get(), container_name, backup_name,
                     opt_md5)) {
      return EXIT_FAILURE;
    }
  } else if (opt_mode == MODE_GET) {
    if (!xbcloud_download(object_store.get(), container_name, backup_name)) {
      return EXIT_FAILURE;
    }
  } else if (opt_mode == MODE_DELETE) {
    if (!xbcloud_delete(object_store.get(), container_name, backup_name)) {
      return EXIT_FAILURE;
    }
  } else {
    msg_ts("Unknown command supplied.\n");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
