#include "httpd.h"


int main(int c, char **v) {
  if (c == 1)
    serve_forever("8000");
  else
    serve_forever(v[1]);
  return 0;
}

void route() {
  ROUTE_START()

  ROUTE_GET("/") {
    printf("HTTP/1.1 200 OK\r\n\r\n");
    printf("Hello! You are using %s", request_header("User-Agent"));
  }

  ROUTE_GET("/test") {
    printf("HTTP/1.1 200 OK\r\n\r\n");
    printf("List of request headers:\r\n\r\n");

    header_t *h = request_headers();

    while (h->name) {
      printf("%s: %s\n", h->name, h->value);
      h++;
    }
  }

  ROUTE_GET("/txt"){
    char filename[100];
    sprintf(filename, "files/%s",qs);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL){
      printf("HTTP/1.1 404\r\n\r\n");
    }
    else {
      printf("HTTP/1.1 200 OK\r\n\r\n");
      char c;
      while (fscanf(fp, "%c", &c) != EOF){
        printf("%c",c);
      }
    }
  }

  ROUTE_POST("/") {
    printf("HTTP/1.1 200 OK\r\n\r\n");
    printf("Wow, seems that you POSTed %d bytes. \r\n", payload_size);
    printf("Fetch the data using `payload` variable.");
  }

  ROUTE_END()
}
