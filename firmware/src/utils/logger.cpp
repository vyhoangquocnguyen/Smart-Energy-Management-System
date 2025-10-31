#include "logger.h"
#include <stdarg.h>

void vlog(const char *prefix, const char *fmt, va_list ap) {
  Serial.print(prefix);
  Serial.print(" ");
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  Serial.println(buf);
}

void log_info(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vlog("[INFO]", fmt, ap);
  va_end(ap);
}
void log_warn(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vlog("[WARN]", fmt, ap);
  va_end(ap);
}
void log_error(const char *fmt, ...) {
  va_list ap; va_start(ap, fmt);
  vlog("[ERROR]", fmt, ap);
  va_end(ap);
}
