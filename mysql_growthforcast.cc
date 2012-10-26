#include <my_global.h>
extern "C" {
#include <my_sys.h>
#include <mysql.h>

#include <curl/curl.h>
}

#include <string>

#define USER_AGENT "mysql_growthforcast/0.01"

extern "C" {

my_bool growthforcast_init(UDF_INIT* initid, UDF_ARGS* args, char* message);
void growthforcast_deinit(UDF_INIT* initid);
char* growthforcast(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length, char* is_null, char* error);

}

typedef struct context {
  CURL *curl;
  char *data;
} CTX;


static size_t writedata(void *ptr, size_t size, size_t nmemb, void *stream)
{
  return 0;
}


my_bool growthforcast_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
  CURL *curl;
  CTX *ctx;

  if (args->arg_count < 1 || args->arg_count > 2)
  {
    strncpy(message, "growthforcast: required one or two argument", MYSQL_ERRMSG_SIZE);
    return 1;
  }
  if (args->arg_type[0] != STRING_RESULT)
  {
    strncpy(message, "growthforcast: argument should be a url", MYSQL_ERRMSG_SIZE);
    return 1;
  }

  args->arg_type[1]   = INT_RESULT;
  initid->maybe_null = 1;

  curl = curl_easy_init();
  if (!curl)
  {
    strncpy(message, "growthforcast: curl init fail", MYSQL_ERRMSG_SIZE);
    return 1;
  }

  curl_easy_setopt(curl, CURLOPT_UPLOAD, 0);
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writedata);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);

  ctx = (CTX *) calloc(sizeof(CTX), 1);
  if (!ctx)
  {
    strncpy(message, "growthforcast: context allocation error", MYSQL_ERRMSG_SIZE);
    return 1;
  }
  ctx->curl = curl;
  ctx->data = NULL;

  initid->ptr        = (char *)(void *)ctx;
  initid->const_item = 1;

  return 0;
}


void growthforcast_deinit(UDF_INIT* initid)
{
  CTX *ctx = (CTX *)(void *)initid->ptr;
  curl_easy_cleanup(ctx->curl);
  if (ctx->data != NULL) {
    free(ctx->data);
  }
  free(ctx);
}


char* growthforcast(UDF_INIT* initid, UDF_ARGS* args, char* result, unsigned long* length, char* is_null, char* error)
{
  CTX *ctx;
  CURLcode status;
  size_t postdata_length;
  long long num;

  ctx = (CTX *) initid->ptr;

  if (args->lengths[0] == 0)
    goto error;

  ctx->data = (char *) malloc(1024);
  if (!ctx->data)
    goto error;

  num = *((long long *) args->args[1]);
  sprintf(ctx->data, "number=%d", (int) num);

  curl_easy_setopt(ctx->curl, CURLOPT_WRITEDATA, ctx);
  curl_easy_setopt(ctx->curl, CURLOPT_URL, args->args[0]);
  curl_easy_setopt(ctx->curl, CURLOPT_POSTFIELDS, ctx->data);

  status = curl_easy_perform(ctx->curl);

  sprintf(ctx->data, "POST %s number=%d", args->args[0], (int) num);

  postdata_length = strlen(ctx->data);
  *length = postdata_length;
  return ctx->data;

  error:
    *is_null = 1;
    ctx->data = NULL;
    //*error = 1;                                                                                                                                                                  
    return NULL;
}
