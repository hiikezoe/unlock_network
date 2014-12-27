/*
 * Copyright (C) 2014 Hiroyuki Ikezoe
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define _LARGEFILE64_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <sys/system_properties.h>

#define OEMINFO_DEVICE "/dev/block/platform/msm_sdcc.1/by-name/oeminfo"
#define OEMINFO_LENGTH 0x200000

static int
search(const char *buffer, ssize_t length)
{
  int i;

  for (i = 0; i < length; i++) {
    if (buffer[i]      == 0x01 &&
        buffer[i +  1] == 0x00 &&
        buffer[i +  2] == 0x3f &&
        buffer[i +  3] == 0x01 &&
        buffer[i +  4] == 0x00 &&
        buffer[i +  5] == 0x10 &&
        buffer[i + 22] == 0x02 &&
        buffer[i + 23] == 0x00 &&
        buffer[i + 24] == 0x10 &&
        buffer[i + 41] == 0x03 &&
        buffer[i + 42] == 0x00 &&
        buffer[i + 43] == 0x10) {
      return i + 6;
    }
  }
  return -1;
}

static bool
write_unlock_keys(char *data)
{
  if (!memcmp(data, data + 19, 16) &&
      !memcmp(data, data + 38, 16)) {
    return false;
  }

  memcpy(data + 19, data, 16);
  memcpy(data + 38, data, 16);

  return true;
}

static bool
unlock(void)
{
  int fd;
  char *data;
  int found_position;

  fd = open(OEMINFO_DEVICE, O_RDWR);
  if (fd == -1) {
    return false;
  }

  data = mmap(NULL, OEMINFO_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    close(fd);
    return false;
  }

  found_position = search(data, OEMINFO_LENGTH);
  if (found_position < 0) {
    printf("This device has unknown oeminfo.\n");
    close(fd);
    return false;
  }

  if (!write_unlock_keys(data + found_position)) {
    printf("This device has already unlocked.\n");
    close(fd);
    return false;
  }

  if (msync(data, OEMINFO_LENGTH, MS_SYNC | MS_INVALIDATE) == -1) {
    close(fd);
    return false;
  }

  if (munmap(data, OEMINFO_LENGTH) == -1) {
    close(fd);
    return false;
  }

  close(fd);

  return true;
}

int
main(int argc, char **argv)
{
  char device[PROP_VALUE_MAX];

  __system_property_get("ro.product.model", device);

  if (strcmp(device, "201HW")) {
    printf("This device is not HUAWEI 201HW\n");
    exit(EXIT_SUCCESS);
  }

  if (!unlock()) {
    if (errno) {
      perror("Failed to unlock " OEMINFO_DEVICE);
    }
    exit(EXIT_FAILURE);
  }

  printf("Unlocked!\n");

  exit(EXIT_SUCCESS);
}
/*
vi:ts=2:nowrap:ai:expandtab:sw=2
*/
