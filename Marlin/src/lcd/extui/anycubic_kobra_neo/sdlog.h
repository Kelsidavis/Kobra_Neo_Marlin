#pragma once
#include "../../../sd/cardreader.h"
#include "../../../inc/MarlinConfig.h"
#include <string.h>
#include <stdio.h>

inline void sdlog(const char *msg) {
  if (!card.isMounted()) return;
  MediaFile file, root = card.getroot();
  if (file.open(&root, "debug.txt", O_CREAT | O_APPEND | O_WRITE)) {
    char line[120];
    int n = snprintf(line, sizeof(line), "[%lu] %s\n", (unsigned long)millis(), msg);
    if (n > 0) file.write(line, n);
    file.close();
  }
}
